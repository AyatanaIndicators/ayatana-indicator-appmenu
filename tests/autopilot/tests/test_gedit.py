
from unity.emulators.unity import Unity
from unity.tests import UnityTestCase
from testtools.matchers import NotEquals, Equals
from autopilot.matchers import Eventually
from autopilot.introspection.gtk import GtkIntrospectionTestMixin



class GeditTestCase(UnityTestCase, GtkIntrospectionTestMixin):

    def setUp(self):
        super(GeditTestCase, self).setUp()
        self.panel_controller = Unity.get_root_instance().panels
        self.app = self.launch_test_application('gedit')


    def guess_quit_location(self, file_menu):
        """Returns an (x, y) location on which to click the 'Quit' menu item."""

        # TODO autopilot-gtk could grok location of GtkImageMenuItem
        geometry = file_menu.menu_geometry
        lower_margin_click_offset = 20
        return (geometry[0] + int(0.5 * geometry[2]),
                geometry[1] + geometry[3] - lower_margin_click_offset)


    def test_file_quit(self):
        """Clicking on 'Quit' exits the application."""

        # open the file menu
        panel = self.panel_controller.get_active_panel()
        file_menu = lambda : panel.menus.get_menu_by_label('_File')
        self.assertThat(file_menu, Eventually(NotEquals(None)))
        file_menu = panel.menus.get_menu_by_label('_File')
        file_menu.mouse_click()

        # click on 'Quit'
        file_menu_is_open = lambda : file_menu.menu_geometry != (0, 0, 0, 0)
        self.assertThat(file_menu_is_open, Eventually(Equals(True)))
        location = self.guess_quit_location(file_menu)
        self.mouse.move(location[0], location[1])
        self.mouse.click()
        
        # make sure we exited
        app_is_running = lambda : self.app_is_running('Text Editor')
        self.assertThat(app_is_running, Eventually(Equals(False)))
