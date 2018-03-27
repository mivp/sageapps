import pydecklink
import time

class PyCallback(pydecklink.Callback):

    def __init__(self):
        pydecklink.Callback.__init__(self)

    def run(self, msg, length):
        print "PyCallback.run()", pydecklink.cdata(msg, length)


dl = pydecklink.DeckLinkManager()
dl.setCallback(PyCallback().__disown__())

time.sleep(1)

dl.StopCapture()
dl.delCallback()