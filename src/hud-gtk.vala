namespace HudGtk {
	public class CellRendererVariant : Gtk.CellRendererText {
		public Variant value {
			set {
				if (value != null) {
					text = value.print (false);
				} else {
					text = "(null)";
				}
			}
		}
	}

	class Window : Gtk.ApplicationWindow {
		DBusConnection session;
		Gtk.ListStore model;
		Variant? query_key;

		void populate_model (Variant update) {
				foreach (var result in update.get_child_value (1)) {
					Gtk.TreeIter iter;

					model.append (out iter);
					for (var i = 0; i < 5; i++) {
						model.set (iter, i, result.get_child_value (i).get_string ());
					}
					model.set (iter, 5, result.get_child_value (5).get_variant ());
				}
				query_key = update.get_child_value (2);
		}

		void updated_query (DBusConnection connection, string sender_name, string object_path, string interface_name, string signal_name, Variant parameters) {
			if (query_key != null && parameters.get_child_value (2).equal (query_key)) {
				model.clear ();
				populate_model (parameters);
			}
		}

		void entry_text_changed (Object object, ParamSpec pspec) {
			var entry = object as Gtk.Entry;

			if (query_key != null) {
				try {
					session.call_sync ("com.canonical.hud", "/com/canonical/hud", "com.canonical.hud",
					                   "CloseQuery", new Variant ("(v)", query_key), null, 0, -1, null);
				} catch (Error e) {
					warning (e.message);
				}
			}

			query_key = null;
			model.clear ();

			try {
				var session = Bus.get_sync (BusType.SESSION, null);
				var reply = session.call_sync ("com.canonical.hud", "/com/canonical/hud", "com.canonical.hud",
				                               "StartQuery", new Variant ("(si)", entry.text, 15),
				                               new VariantType ("(sa(sssssv)v)"), 0, -1, null);
				populate_model (reply);
			} catch (Error e) {
				warning (e.message);
			}
		}

		void view_activated (Gtk.TreeView view, Gtk.TreePath path, Gtk.TreeViewColumn column) {
			Gtk.TreeIter iter;
			Variant key;

			model.get_iter (out iter, path);
			model.get (iter, 5, out key);

			try {
				session.call_sync ("com.canonical.hud", "/com/canonical/hud", "com.canonical.hud",
				                   "ExecuteQuery", new Variant ("(vu)", key, 0), null, 0, -1, null);
			} catch (Error e) {
				warning (e.message);
			}
		}

		public Window (Gtk.Application application) {
			Object (application: application, title: "Hud");
			set_default_size (500, 300);

			var builder = new Gtk.Builder ();
			try {
				new CellRendererVariant ();
				session = Bus.get_sync (BusType.SESSION, null);
				builder.add_from_file ("hud-gtk.ui");
			} catch (Error e) {
				error (e.message);
			}

			session.signal_subscribe ("com.canonical.hud", "com.canonical.hud", "UpdatedQuery",
			                          "/com/canonical/hud", null, DBusSignalFlags.NONE, updated_query);
			model = builder.get_object ("liststore") as Gtk.ListStore;
			builder.get_object ("entry").notify["text"].connect (entry_text_changed);
			(builder.get_object ("treeview") as Gtk.TreeView).row_activated.connect (view_activated);
			add (builder.get_object ("grid") as Gtk.Widget);
		}
	}

	class Application : Gtk.Application {
		protected override void activate () {
			new Window (this).show_all ();
		}

		public Application () {
			Object (application_id: "com.canonical.HudGtk");
		}
	}
}

int main (string[] args) {
	return new HudGtk.Application ().run (args);
}
