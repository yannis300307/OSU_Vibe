#pragma once

#include "geode_includes.h"
#include "globals.h"

class $modify(MyAppDelegate, AppDelegate)
{
    public:
        void applicationWillEnterForeground();
    
        // Keep the music playing in the background
        void applicationDidEnterBackground();
};