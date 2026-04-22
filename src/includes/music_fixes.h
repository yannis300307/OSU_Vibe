#pragma once

#include "geode_includes.h"
#include "globals.h"
#include <Geode/modify/CCDirector.hpp>

// Adapted from https://github.com/RayDeeUx/menuloop_randomizer/blob/main/src/CCDirector.cpp

class $modify(MyCCDirector, CCDirector) {
	void willSwitchToScene(cocos2d::CCScene* scene);
	bool pushScene(cocos2d::CCScene* scene);
	bool replaceScene(cocos2d::CCScene* scene);
	void popSceneWithTransition(float p0, cocos2d::PopTransition p1);
};