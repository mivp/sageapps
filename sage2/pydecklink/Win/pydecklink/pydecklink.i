// ref: http://www.matthiaskauer.com/2013/08/tutorial-mixing-c-and-python-on-windows-with-swig-and-visual-studio-express-2012/

%module(directors="1") pydecklink

%include "cdata.i"

%{
#include "DeckLinkManager.h"
%}

%feature("director") Callback;

//double-check that this is indeed %include !!!
%include "DeckLinkManager.h"
