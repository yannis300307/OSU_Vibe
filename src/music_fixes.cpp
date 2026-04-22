#include "includes/music_fixes.h"

void MyCCDirector::willSwitchToScene(cocos2d::CCScene* scene) {
    CCDirector* director = CCDirector::get();
    CCScene* previousScene = director->getRunningScene();

    director->willSwitchToScene(scene);
}

bool MyCCDirector::pushScene(cocos2d::CCScene* scene) {
    CCDirector* director = CCDirector::get();
    CCScene* previousScene = director->getRunningScene();

    bool result = director->pushScene(scene);

    return result;
}

bool MyCCDirector::replaceScene(cocos2d::CCScene* scene) {
    CCDirector* director = CCDirector::get();
    CCScene* previousScene = director->getRunningScene();

    bool result = director->replaceScene(scene);

    return result;
}

void MyCCDirector::popSceneWithTransition(float p0, cocos2d::PopTransition p1) {
    CCDirector* director = CCDirector::get();
    CCScene* previousScene = director->getRunningScene();

    director->popSceneWithTransition(p0, p1);
}
