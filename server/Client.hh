
#pragma once

// This is a virtual base class for programs wanting to implement a 
// cadabra client. It contains the document representation, and manipulator
// functions to change it. It also contains the logic to execute a cell,
// that is, lock it, send it to the server, and wait for response.
// When a response comes in, the relevant virtual functions will be called.
// These should implement the necessary logic to update the display.
// On OS X these would then call Cocoa GCD to update the gui, whereas on
// gtkmm they would gdk_thread_enter and do the work.

// Some of the functionality of this class is split off into Action classes 
// in order to provide an undo stack.

//FIX: do not use smart pointers, but instead address cells by iterators, and
//just use the fact that iterators remain valid even when nodes get moved in and 
//			  out of the tree. RemoveCell then just needs to keep track of the removed node.

#include "tree.hh"
#include <stack>

#include <websocketpp/client.hpp>
#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/common/functional.hpp>

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;
typedef websocketpp::client<websocketpp::config::asio_client> wsclient;
typedef websocketpp::config::asio_client::message_type::ptr message_ptr;

namespace cadabra {

	class Client {
		public:
			Client();

			// Main entry point, which will connect to the server and then start an
			// event loop to handle communication with the server. Only exists when
			// the connection drops. Run your GUI on a different thread.

			void run(); 

			// Callback functions to inform the client of a changed state on the server.
			// Implement these in your derived class.

			virtual void on_connect()=0;
			virtual void on_disconnect()=0;			
			virtual void on_network_error()=0;
			virtual void on_progress()=0;

			class ActionBase;

			// The client is informed about any change to the document tree using the
			// following callback. Events like deleting cells will be signalled before
			// the change is made, others like adding cells will be signalled after the
			// fact. Changes to the GUI should _only_ be made in response to these
			// callbacks.

			virtual void before_tree_change(ActionBase&)=0;
			virtual void after_tree_change(ActionBase&)=0;

			// All modifications to the document are done by calling 'perform' with an 
			// action object. This enables us to implement an undo stack. 

			void perform(const ActionBase&);

			// DataCells are the basic building blocks for a document. They are stored 
			// in a tree inside the client. A user interface should read these cells
			// and construct corresponding graphical output for them.

			class DataCell {
				public:
					enum class CellType { input, output, comment, texcomment, tex, error };
					
					DataCell(CellType t=CellType::input, const std::string& str="", bool texhidden=false);
					
					CellType                      cell_type;
					std::string                   textbuf;
					std::string                   cdbbuf;             // c_output only: the output in cadabra input format
					bool                          tex_hidden;         // c_tex only
					bool                          sensitive;
					bool                          running;
			};
			
			// The document is a tree of DataCells. A read-only version of this document
			// is available from the 'dtree' function. All changes to the tree should be
			// made by submitting ActionBase derived objects to the 'perform' function,
			// so that an undo stack can be kept.

			typedef tree<DataCell>           DTree;
			typedef tree<DataCell>::iterator iterator;

			const DTree& dtree();

			// The Action object is used to pass user action instructions around
         // and store them in the undo/redo stacks. All references to cells is
         // in terms of smart pointers to DataCells. 

         // This requires that if we delete a cell, its data cell together
			// with any TextBuffer and TeXBuffer objects should be kept in
			// memory, so that the pointer remains valid.  We keep a RefPtr.

			class ActionBase {
				public:
					ActionBase(iterator);
					
					virtual void execute()=0;
					virtual void revert()=0;
					
					iterator cell;
			};
			

//			class ActionAddCell : public ActionBase {
//				public:
//					enum class Position { before, after, child };
//
//					ActionAddCell(tree<DataCell>::iterator, tree<DataCell>::iterator ref_, Position pos_);
//					
//					/// Executing will also show the cell and grab its focus.
//					virtual void execute(XCadabra&);
//					virtual void revert(XCadabra&);
//					
//				private:
//					// Keep track of the location where this cell is inserted into
//					// the notebook. 
//
//					tree<DataCell>::iterator  ref;
//					Position                  position;
//			};
//			
//			class ActionRemoveCell : public ActionBase {
//				public:
//					ActionRemoveCell(tree<DataCell>::iterator);
//					~ActionRemoveCell();
//					
//					virtual void execute(XCadabra&);
//					virtual void revert(XCadabra&);
//					
//				private:
//					// Keep track of the location where this cell was in the notebook. Since it is
//					// not possible to delete the first cell, it is safe to keep a reference to the
//					// cell just before the one we are deleting. 
//
//					tree<DataCell>::iterator             prev_cell;
//			};
//			
//			class ActionAddText : public ActionBase {
//				public:
//					ActionAddText(tree<DataCell>::iterator, int, const std::string&);
//					
//					virtual void execute(XCadabra&);
//					virtual void revert(XCadabra&);
//					
//					int         insert_pos;
//					std::string text;
//			};
//			
//			class ActionRemoveText : public ActionBase {
//				public:
//					ActionRemoveText(tree<DataCell>::iterator, int, int, const std::string&);
//					
//					virtual void execute(XCadabra&);
//					virtual void revert(XCadabra&);
//					
//					int from_pos, to_pos;
//					std::string removed_text;
//			};
//
//			// todo: split cell, execute cell (or should the latter be a normal, non-undoable function?)
			
		private:

			// WebSocket++ callbacks.
			void on_open(wsclient* c, websocketpp::connection_hdl hdl);
			void on_fail(wsclient* c, websocketpp::connection_hdl hdl);
			void on_close(wsclient* c, websocketpp::connection_hdl hdl);
			void on_message(wsclient* c, websocketpp::connection_hdl hdl, message_ptr msg);

			// The actual document and the action that led to it.
			DTree doc;
			typedef std::stack<std::unique_ptr<ActionBase> > ActionStack;
	};

}
