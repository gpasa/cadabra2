
#pragma once

#include <gtkmm/window.h>
#include <gtkmm/box.h>
#include <gtkmm/progressbar.h>
#include <gtkmm/label.h>
#include <gtkmm/stock.h>
#include <gtkmm/button.h>
#include <glibmm/dispatcher.h>

#include <thread>
#include <mutex>

#include "Client.hh"
#include "GUIBase.hh"

namespace cadabra {

// Each notebook has one main window which controls it. It has a menu bar, a
// status pane and one or more panels that represent a view on the document.

   class NotebookWindow : public Gtk::Window, public GUIBase {
      public:
         NotebookWindow();
         ~NotebookWindow();
			
         // Methods used to communicate information from Netbits to the GUI.
			// These get called on the Netbits thread, which is _not_ the GUI
			// thread, so they store the information (FIXME: where) and then
			// signal the GUI thread through the use of the dispatcher (below).
			// All actual GUI updates take place in the on_netbits_notification
			// member, on the GUI thread.
			
         void on_connect();
         void on_disconnect();


			// Implementations of the GUIBase interface for updating the document.
			
			virtual void add_cell(Client::iterator);
			virtual void remove_cell(Client::iterator);


      private:
			// Main handler which fires whenever the Netbits object signals 
			// that the document is changing or the network status is modified.
			// Runs on the GUI thread.

			Glib::Dispatcher dispatcher;
			void on_netbits_notification();

			// Modifications which are not yet reflected in the GUI representation.
			// These need to be processed before Netbits can be asked for a new
			// change.

			std::mutex                              todo_mutex;
			std::deque<cadabra::Client::ActionBase> todo_stack;


			// GUI elements.
			
			Glib::RefPtr<Gtk::ActionGroup> actiongroup;

			Gtk::VBox                      topbox;
			Gtk::HBox                      supermainbox;
			Gtk::VBox                      mainbox;
			Gtk::HBox                      buttonbox;
			Gtk::HBox                      statusbarbox;

         // Buttons
         Gtk::Button                    b_kill, b_run, b_run_to, b_run_from, b_help, b_stop, b_undo, b_redo;


			// Status bar
			Gtk::ProgressBar               progressbar;
			Gtk::Label                     cdbstatus, kernelversion;

			// Name and modification data.
			void             update_title();
			std::string      name;
			bool             modified;
	};

};
