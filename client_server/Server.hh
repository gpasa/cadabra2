
#pragma once

#include <string>
#include <boost/uuid/uuid.hpp>
#include <websocketpp/server.hpp>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/common/functional.hpp>
#include <future>
#include <boost/python.hpp>

/// \ingroup clientserver
///
/// Object representing a Cadabra server, capable of receiving messages
/// on a websocket, running Python code, and sending output back to the
/// client.
///
/// Contains the logic to intercept raw Python output but also provides
/// functionality to the Python side which enables clients to send 
/// various objects in JSON encoded form. See Server::on_block_finished
/// for the format of these messages.
///
/// Every block is run on the same Python scope. They run sequentially,
/// one at a time, on a thread separate from the websocket++ main loop.
/// When the Python code finishes (or when it is interrupted), this thread
/// locks the socket_mutex and calls on_block_finished().

class Server {
	public:
		Server();
		Server(const Server&)=delete;
		Server(const std::string& socket);
		~Server();

		/// The only user-visible part: just instantiate a server object and
		/// start it with run(). This will not return until the server has
		/// been shut down.
		void run();


		/// Python output catching. We implement this in a C++ class
		/// because we want to be able to catch each line of output
		/// separately, and perhaps add additional functionality to send
		/// out-of-band messages to the client later.

		class CatchOutput {
			public:
				CatchOutput();
				CatchOutput(const CatchOutput&);

				void        write(const std::string& txt);
				void        clear();
				std::string str() const;
			private:
				std::string collect;
		};

		CatchOutput catchOut, catchErr;		

	private:
		void init();

		// WebSocket++ dependent parts below.
		typedef websocketpp::server<websocketpp::config::asio> WebsocketServer;
		void on_socket_init(websocketpp::connection_hdl hdl, boost::asio::ip::tcp::socket & s);
		void on_message(websocketpp::connection_hdl hdl, WebsocketServer::message_ptr msg);
		void on_open(websocketpp::connection_hdl hdl);
		void on_close(websocketpp::connection_hdl hdl);
		WebsocketServer wserver;
		std::string     socket_name;

		// Connection tracking.  There can be multiple connections to
		// the server, but they all have access to the same Python
		// scope. With multiple connections, one can inspect the Python
		// stack from a different client (e.g. for debugging purposes).

		class Connection {
			public:
				Connection();

				websocketpp::connection_hdl hdl;
				boost::uuids::uuid          uuid;
		};
		typedef std::map<websocketpp::connection_hdl, Connection, 
							  std::owner_less<websocketpp::connection_hdl>> ConnectionMap;
		ConnectionMap connections;

		// Mutex to be able to use the websocket layer from both the
		// main loop and the python-running thread.
		std::mutex ws_mutex;


		// Basics for the working thread that processes blocks.
		std::thread             runner;
		std::mutex              block_available_mutex;
		std::condition_variable block_available;
		void wait_for_job();

		// Data and connection info for a single block of code.
		class Block {
			public:
				Block(websocketpp::connection_hdl, const std::string&, uint64_t id);
				websocketpp::connection_hdl hdl; // FIXME: decouple from websocket?
				std::string                 input;
				std::string                 output;
				std::string                 error;
				uint64_t                    cell_id;
		};
		std::queue<Block>           block_queue;
		websocketpp::connection_hdl current_hdl;
		uint64_t                    current_id;

		// Run a piece of Python code. This is called from a separate
		// thread constructed by on_message().
		std::string              run_string(const std::string&, bool handle_output=true);

		// Pre-parse a line of code to convert Cadabra notation into proper
		// Python. Mimics the functionality in core/cadabra2.
		std::string              pre_parse(const std::string&);

		// Escape single and double quotes so they can be fed into Python strings.
		std::string              escape_quotes(const std::string& line);

		std::string indent, lhs, rhs;

		/// Called by the run_block() thread upon completion of the
		/// task.  Handles communication of the result back to the
		/// client in JSON format. This is always of the form
		///
		///      { "header":   { "parent_id":     "...",
		///                      "parent_origin": "client" | "server", 
		///                      "cell_id":       "...", 
		///                      "cell_origin":   "client" | "server"
      ///                    },
      ///        "content":  { "output":  "..." },
      ///        "msg_type": "..." 
      ///      }
		///
		/// msg_type can be "output", "latex", "image_png" and so on,
		/// corresponding to the possible values of DataCell::CellType.

		void                     on_block_finished(Block);
		void                     on_block_error(Block);
		void                     on_kernel_fault(Block);

		/// Raw code to send a string (which must be JSON formatted) as
		/// a message to the client.
		void                     send(const std::string& output, const std::string& msg_type);
		void                     send_json(const std::string&);

		/// Halt the currently running block and prevent execution of any
		/// further blocks that may still be on the queue.
		void                     stop_block(); 
		bool                     started;
		std::future<std::string> job;

		/// Takes a JSON encoded message and performs the required action to process it.
		/// Where applicable these messages are compatible with IPython's message types,
		/// http://ipython.org/ipython-doc/dev/development/messaging.html

		void dispatch_message(websocketpp::connection_hdl, const std::string& json_string);

		// Python global info.
		boost::python::object main_module;
		boost::python::object main_namespace;

};

