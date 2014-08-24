/*
-------------------------------------------------------------------------
 CxxTest: A lightweight C++ unit testing library.
 Copyright (c) 2008 Sandia Corporation.
 This software is distributed under the LGPL License v3
 For more information, see the COPYING file in the top CxxTest directory.
 Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
 the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------
*/

#ifndef __CXXTEST__GUI_H
#define __CXXTEST__GUI_H

//
// GuiListener is a simple base class for the differes GUIs
// GuiTuiRunner<GuiT, TuiT> combines a GUI with a text-mode error formatter
//

#include <cxxtest/TeeListener.h>

namespace CxxTest
{
class GuiListener : public TestListener
{
public:
    GuiListener() : _state(GREEN_BAR) {}
    virtual ~GuiListener() {}

    virtual void runGui(TestListener &listener)
    {
        enterGui();
        TestRunner::runAllTests(listener);
        leaveGui();
    }

    virtual void enterGui() {}
    virtual void leaveGui() {}

    //
    // The easy way is to implement these functions:
    //
    virtual void guiEnterWorld(unsigned /*numTotalTests*/) {}
    virtual void guiEnterSuite(const char * /*suiteName*/) {}
    virtual void guiEnterTest(const char * /*suiteName*/, const char * /*testName*/) {}
    virtual void yellowBar() {}
    virtual void redBar() {}

    //
    // The hard way is this:
    //
    void enterWorld(const WorldDescription &d) { guiEnterWorld(d.numTotalTests()); }
    void enterSuite(const SuiteDescription &d) { guiEnterSuite(d.suiteName()); }
    void enterTest(const TestDescription &d) { guiEnterTest(d.suiteName(), d.testName()); }
    void leaveTest(const TestDescription &) {}
    void leaveSuite(const SuiteDescription &) {}
    void leaveWorld(const WorldDescription &) {}

    void warning(const char * /*file*/, int /*line*/, const char * /*expression*/)
    {
        yellowBarSafe();
    }

    void skippedTest(const char * /*file*/, int /*line*/, const char * /*expression*/)
    {
        yellowBarSafe();
    }

    void failedTest(const char * /*file*/, int /*line*/, const char * /*expression*/)
    {
        redBarSafe();
    }

    void failedAssert(const char * /*file*/, int /*line*/, const char * /*expression*/)
    {
        redBarSafe();
    }

    void failedAssertEquals(const char * /*file*/, int /*line*/,
                            const char * /*xStr*/, const char * /*yStr*/,
                            const char * /*x*/, const char * /*y*/)
    {
        redBarSafe();
    }

    void failedAssertSameData(const char * /*file*/, int /*line*/,
                              const char * /*xStr*/, const char * /*yStr*/,
                              const char * /*sizeStr*/, const void * /*x*/,
                              const void * /*y*/, unsigned /*size*/)
    {
        redBarSafe();
    }

    void failedAssertDelta(const char * /*file*/, int /*line*/,
                           const char * /*xStr*/, const char * /*yStr*/, const char * /*dStr*/,
                           const char * /*x*/, const char * /*y*/, const char * /*d*/)
    {
        redBarSafe();
    }

    void failedAssertDiffers(const char * /*file*/, int /*line*/,
                             const char * /*xStr*/, const char * /*yStr*/,
                             const char * /*value*/)
    {
        redBarSafe();
    }

    void failedAssertLessThan(const char * /*file*/, int /*line*/,
                              const char * /*xStr*/, const char * /*yStr*/,
                              const char * /*x*/, const char * /*y*/)
    {
        redBarSafe();
    }

    void failedAssertLessThanEquals(const char * /*file*/, int /*line*/,
                                    const char * /*xStr*/, const char * /*yStr*/,
                                    const char * /*x*/, const char * /*y*/)
    {
        redBarSafe();
    }

    void failedAssertPredicate(const char * /*file*/, int /*line*/,
                               const char * /*predicate*/, const char * /*xStr*/, const char * /*x*/)
    {
        redBarSafe();
    }

    void failedAssertRelation(const char * /*file*/, int /*line*/,
                              const char * /*relation*/, const char * /*xStr*/, const char * /*yStr*/,
                              const char * /*x*/, const char * /*y*/)
    {
        redBarSafe();
    }

    void failedAssertThrows(const char * /*file*/, int /*line*/,
                            const char * /*expression*/, const char * /*type*/,
                            bool /*otherThrown*/)
    {
        redBarSafe();
    }

    void failedAssertThrowsNot(const char * /*file*/, int /*line*/,
                               const char * /*expression*/)
    {
        redBarSafe();
    }

protected:
    void yellowBarSafe()
    {
        if (_state < YELLOW_BAR)
        {
            yellowBar();
            _state = YELLOW_BAR;
        }
    }

    void redBarSafe()
    {
        if (_state < RED_BAR)
        {
            redBar();
            _state = RED_BAR;
        }
    }

private:
    enum { GREEN_BAR, YELLOW_BAR, RED_BAR } _state;
};

template<class GuiT, class TuiT>
class GuiTuiRunner : public TeeListener
{
    GuiT _gui;
    TuiT _tui;

public:
    GuiTuiRunner() {}

    bool process_commandline_args(int& i, int& argc, char* argv[])
    {
        if (i == 0) // Init
        {
            setFirst(_gui);
            setSecond(_tui);
        }
        return _gui.process_commandline_args(i, argc, argv);
    }

    void print_help(const char* name)
    {
        _gui.print_help(name);
    }

    int run()
    {
        _gui.runGui(*this);
        return tracker().failedTests();
    }
};
}

#endif //__CXXTEST__GUI_H

