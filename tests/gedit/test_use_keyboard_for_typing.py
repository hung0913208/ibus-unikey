import unittest, os, signal, sys

from dogtail import tree
from dogtail.config import config
from dogtail.procedural import focus, click, run, FocusWidget, FocusDialog, FocusWindow, FocusApplication
from dogtail.rawinput import typeText, keyCombo
from dogtail.utils import screenshot
from dogtail.predicate import GenericPredicate

# @NOTE: i'm considering move these function to be as gedit helpers so we
# can reuse them to many more test-cases

method = 'telex'

def run_ui_typing_helper(cache, text):
    if os.path.isfile(cache):
        os.remove(cache)

    pid = run('gedit')
    gedit = tree.root.application('gedit')
    saved = False
    ready_to_save = False

    try:
        focus.application('gedit')
     
        focus.text()
        typeText(text)

        # Click gedit's Save button.
        click.button('Save')

        try:
            # Focus gedit's Save As... dialog

            focus.widget.findByPredicate(GenericPredicate(roleName='file chooser'))
        finally:
            ready_to_save = True
    except FocusError as error:
        try:
            # This string changed somewhere around gedit 2.13.2.
            # This is the new string

            focus.dialog('Save As\u2026')
        except FocusError:
            # Fall back to the old string.

            focus.dialog('Save as...')
        finally:
            ready_to_save = True
    finally:
        try:
            if ready_to_save:
                try:
                    typeText(cache)
                    click('Save')
                finally:
                    saved = True

            focus.application('gedit')

            # Let's quit now.
            try:
                # @NOTE: maybe we can't access the menu and click the item `Quit`
                # at menu `File`

                click.menu('File')
                click.menuItem('Quit')
            except Exception as error:
                keyCombo('<Control>q')
        except Exception as error:
            print('can\'t quit app, here is the reason: {}'.format(error))
            os.kill(pid, signal.SIGKILL)

    if saved:
        with open(cache) as fd:
            return fd.read()
    else:
        return None

class TestKeyboardTyping(unittest.TestCase):
    def __init__(self, *args, **kwargs):
        super(TestKeyboardTyping, self).__init__(*args, **kwargs)

    def test_typing_vietnamese_style(self):
        if method == 'telex':
            steps = [
                ('xin chaof, tooi laf mootj con autobot đuowcj xaya dungj ddeer tesst ibus-unikey',
                 'xin chào, tôi là một con autobot được xây dựng để test ibus-unikey\n')
            ]
        elif method == 'vni':
            steps = [
                ('xin chao2, to6i la2 mo65t con autobot d9uo75c xa6y du7ng5 d9e63 test ibus-unikey',
                 'xin chào, tôi là một con autobot được xây dựng để test ibus-unikey\n')
            ]

        for typing, expect in steps:
            result = run_ui_typing_helper('/tmp/0001.txt', typing)
            self.assertEqual(result, expect)

if __name__ == '__main__':
    #import dogtail.i18n
    #dogtail.i18n.loadTranslationsFromPackageMoFiles('gedit')

    #config.debugSleep = True
    #config.debugSearching = True
    #config.debugTranslation = True

    if len(sys.argv) > 1:
        method = sys.argv.pop()
    else:
        method = 'telex'
    unittest.main()
