
#include "LHSceneGearJointDemo.h"

LHSceneGearJointDemo* LHSceneGearJointDemo::create()
{
    LHSceneGearJointDemo *ret = new LHSceneGearJointDemo();
    if (ret && ret->initWithContentOfFile("DEMO_PUBLISH_FOLDER/gearJointDemo.lhplist"))
    {
        ret->autorelease();
        return ret;
    }
    else
    {
        CC_SAFE_DELETE(ret);
        return nullptr;
    }
}

LHSceneGearJointDemo::LHSceneGearJointDemo()
{
    /*INITIALIZE YOUR CONTENT HERE*/
    /*AT THIS POINT NOTHING IS LOADED*/
#if LH_USE_BOX2D
    mouseJoint = NULL;
#else
    touchedNode = NULL;
#endif
    
}

LHSceneGearJointDemo::~LHSceneGearJointDemo()
{
    //nothing to release
    this->destroyMouseJoint();
}

std::string LHSceneGearJointDemo::className(){
    return "LHSceneGearJointDemo";
}

bool LHSceneGearJointDemo::initWithContentOfFile(const std::string& plistLevelFile)
{
    bool retValue = LHSceneDemo::initWithContentOfFile(plistLevelFile);
    
    /*INITIALIZE YOUR CONTENT HERE*/
    /*AT THIS POINT EVERYTHING IS LOADED AND YOU CAN ACCESS YOUR OBJECTS*/
    

    Size size = this->getContentSize();
    
    Label* ttf = Label::create();

#if LH_USE_BOX2D
    ttf->setString("Gear Joints Example.\nIn order for gear joints to work, Box2d requires all objects involved to be dynamic.\nDrag a wheel or the red handle to move the joints.");
#else
    ttf->setString("Gear Joints Example.\nSorry, this demo is not available when using Chipmunk.\nPlease switch to Box2d target in Xcode.");
#endif
    
    
    ttf->setTextColor(Color4B::BLACK);
    ttf->setHorizontalAlignment(TextHAlignment::CENTER);
    ttf->setPosition(Point(size.width*0.5, size.height*0.5));
    ttf->setSystemFontSize(20);
    this->getUINode()->addChild(ttf);//add the text to the ui element as we dont want it to move with the camera
    
        
    return retValue;
}

bool LHSceneGearJointDemo::onTouchBegan(Touch* touch, Event* event)
{
    Point touchLocation = touch->getLocation();
    touchLocation = this->getGameWorldNode()->convertToNodeSpace(touchLocation);

    
    this->createMouseJointForTouchLocation(touchLocation);
    
    //dont forget to call super
    return LHScene::onTouchBegan(touch, event);
}
void LHSceneGearJointDemo::onTouchMoved(Touch* touch, Event* event){

    Point touchLocation = touch->getLocation();
    touchLocation = this->getGameWorldNode()->convertToNodeSpace(touchLocation);


    this->setTargetOnMouseJoint(touchLocation);
    
    //dont forget to call super
    LHScene::onTouchMoved(touch, event);
}
void LHSceneGearJointDemo::onTouchEnded(Touch* touch, Event* event){
    
    this->destroyMouseJoint();
    
    LHScene::onTouchEnded(touch, event);
}
void LHSceneGearJointDemo::onTouchCancelled(Touch *touch, Event *event){

    this->destroyMouseJoint();
    
    LHScene::onTouchCancelled(touch, event);
}



void LHSceneGearJointDemo::createMouseJointForTouchLocation(Point point)
{
#if LH_USE_BOX2D
    b2Body* ourBody = NULL;
    
    b2World* world = this->getBox2dWorld();
    
    if(world == NULL)return;
    
    LHNode* mouseJointDummyNode = (LHNode*)this->getChildNodeWithName("dummyBodyForMouseJoint");
    b2Body* mouseJointBody = mouseJointDummyNode->getBox2dBody();
    
    if(!mouseJointBody)return;
    
    b2Vec2 pointToTest = this->metersFromPoint(point);
    for (b2Body* b = world->GetBodyList(); b; b = b->GetNext())
    {
        if(b != mouseJointBody)
        {
            b2Fixture* stFix = b->GetFixtureList();
            while(stFix != 0){
                if(stFix->TestPoint(pointToTest)){
                    if(ourBody == NULL)
                    {
                        ourBody = b;
                    }
                    else{
                        LHNode* ourNode = (LHNode*)(ourBody->GetUserData());
                        LHNode* bNode   = (LHNode*)(b->GetUserData());
                        
                        if(bNode->getLocalZOrder() > ourNode->getLocalZOrder())
                        {
                            ourBody = b;
                        }
                    }
                }
                stFix = stFix->GetNext();
            }
        }
    }
    
    if(ourBody == NULL || mouseJointBody == NULL)
        return;
    
    b2MouseJointDef md;
    md.bodyA = mouseJointBody;
    md.bodyB = ourBody;
    b2Vec2 locationWorld = pointToTest;
    
    md.target = locationWorld;
    md.collideConnected = true;
    md.maxForce = 1000.0f * ourBody->GetMass();
    ourBody->SetAwake(true);
    
    if(mouseJoint){
        world->DestroyJoint(mouseJoint);
        mouseJoint = NULL;
    }
    mouseJoint = (b2MouseJoint*)world->CreateJoint(&md);
    
#endif
}

void LHSceneGearJointDemo::setTargetOnMouseJoint(Point point)
{
#if LH_USE_BOX2D
    if(mouseJoint == 0)
        return;
    
    b2Vec2 locationWorld = b2Vec2(this->metersFromPoint(point));
    mouseJoint->SetTarget(locationWorld);
#endif
}

void LHSceneGearJointDemo::destroyMouseJoint()
{
#if LH_USE_BOX2D
    if(mouseJoint){
        this->getBox2dWorld()->DestroyJoint(mouseJoint);
    }
    mouseJoint = NULL;
#endif
    
}
