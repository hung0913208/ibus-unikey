import unittest, os, signal, sys, time

from dogtail import tree
from dogtail.config import config
from dogtail.procedural import focus, click, run, FocusWidget, FocusDialog, FocusWindow, FocusApplication, FocusError
from dogtail.rawinput import typeText, keyCombo
from dogtail.utils import screenshot
from dogtail.predicate import GenericPredicate

method = 'telex'
app = 'chromium-browser'

def start_ui_typing_helper():
    for _ in range(2):
        try:
            print('run {} using dogtail'.format(app))
            pid = run('{} --force-renderer-accessibility'.format(app))

            print('use tree.root.application to touch to {}'.format(app))
            print('{}'.format(tree.root.__dict__))
            chrome = tree.root.application(app)

            print('focus application {}')
            focus.application('{}')
            focus.text()

            return pid
        except Exception as error:
            print('got error {}, while start {}'.format(error, app))
            screenshot('{}.png'.format(app))
            os.kill(pid, signal.SIGTERM)
    else:
        return None

def save_and_close_ui_helper(pid, cache, instruct):
    if os.path.isfile(cache):
        os.remove(cache)
    try:
        print('do typing with {}'.format(app))
        instruct() 

        print('copy text into clipboard')
        keyCombo('<Ctrl>a')
        keyCombo('<Ctrl>c')
        keyCombo('<Alt>F4')
    except Exception as error:
        print('got error {}, kill everything'.format(error))
        screenshot('{}.png'.format(app))
        return None
    finally:
        for _ in range(60):
            try:
                os.kill(pid, 0)
            except OSError:
                time.sleep(10)
            else:
                print('finish killing {}'.format(app))
                break
        else:
            os.kill(pid, signal.SIGKILL)
            return None

    print('start gedit to save our clipboard')
    pid = run('gedit {}'.format(cache))
    tree.root.application('gedit')

    focus.application('gedit')    
    focus.text()

    print('paster our clipboard and close gedit')
    keyCombo('<Control>v') 
    keyCombo('<Control>s')
    keyCombo('<Control>q')

    try:
        with open(cache) as fd:
            return fd.read()
    except Exception as error:
        screenshot('{}.png'.format(app))
        os.kill(pid, signal.SIGKILL)
        return None

class TestKeyboardTyping(unittest.TestCase):
    def __init__(self, *args, **kwargs):
        super(TestKeyboardTyping, self).__init__(*args, **kwargs)

    def test_typing_vietnamese_while_press_shift(self):
        cache = '/tmp/0000_vietnamese_press_shift.txt'

        if method == 'telex':
            steps = [
                ('duoiws dday', ';', 'dưới đây:')
            ]
        elif method == 'vni':
            steps = [
                ('duo71i d9a6y', ';', 'dưới đây:')
            ]

        for _ in range(loop):
            for typing, shift_char, expect in steps:
                pid = None

                def instruct():
                    typeText(typing)
                    keyCombo('<Shift>_{}'.format(shift_char))

                try:
                    pid = start_ui_typing_helper()

                    self.assertNotEqual(pid, None, "can't start {}".format(app))
                    self.assertEqual(save_and_close_ui_helper(pid, cache, instruct),
                                     expect)
                finally:
                    if pid:
                        os.kill(pid, signal.SIGKILL)

if __name__ == '__main__':
    #import dogtail.i18n
    #dogtail.i18n.loadTranslationsFromPackageMoFiles('gedit')

    #config.debugSleep = True
    #config.debugSearching = True
    #config.debugTranslation = True
    #config.actionDelay = 0.1

    if len(sys.argv) > 3:
        loop = int(sys.argv.pop())
    else:
        loop = 1

    if len(sys.argv) > 2:
        app = sys.argv.pop()
    else:
        app = 'google-chrome'

    if len(sys.argv) > 1:
        method = sys.argv.pop()
    else:
        method = 'telex'

    config.scratchDir = '/tmp/chromium-brower'

    if os.path.exists(config.scratchDir) is False:
        os.mkdir(config.scratchDir)

    unittest.main()
