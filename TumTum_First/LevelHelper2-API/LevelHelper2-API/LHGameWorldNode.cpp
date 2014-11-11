//
//  LHGameWorldNode.cpp
//  LevelHelper2-Cocos2d-X-v3
//
//  Created by Bogdan Vladu on 31/03/14.
//  Copyright (c) 2014 GameDevHelper.com. All rights reserved.
//

#include "LHGameWorldNode.h"
#include "LHDictionary.h"
#include "LHScene.h"
#include "LHDevice.h"
#include "LHConfig.h"

#if LH_USE_BOX2D
#include "LHBox2dDebugDrawNode.h"

#endif



LHGameWorldNode::LHGameWorldNode()
{
#if LH_USE_BOX2D
    _box2dWorld = nullptr;
    _debugNode = nullptr;
#endif
}

LHGameWorldNode::~LHGameWorldNode()
{
#if LH_USE_BOX2D
    //we need to first destroy all children and then destroy box2d world
    
    auto& children = this->getChildren();
    for( const auto &n : children)
    {
        LHNodeProtocol* nProt = dynamic_cast<LHNodeProtocol*>(n);
        if(nProt)
        {
            nProt->markAsB2WorldDirty();
        }
    }
    
    
    _scheduledBeginContact.clear();
    _scheduledEndContact.clear();
    
    this->removeAllChildren();
    
    delete _box2dWorld;
    _box2dWorld = nullptr;
    
    _debugNode = nullptr;
#endif

}

LHGameWorldNode* LHGameWorldNode::nodeWithDictionary(LHDictionary* dict, Node* prnt)
{
    LHGameWorldNode *ret = new LHGameWorldNode();
    if (ret && ret->initWithDictionary(dict, prnt))
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

bool LHGameWorldNode::initWithDictionary(LHDictionary* dict, Node* prnt)
{
    if(Node::init())
    {
        _physicsBody = NULL;
        prnt->addChild(this);
        
#if LH_USE_BOX2D
        FIXED_TIMESTEP = 1.0f / 120.0f;
        MINIMUM_TIMESTEP = 1.0f / 600.0f;
        VELOCITY_ITERATIONS = 8;
        POSITION_ITERATIONS = 8;
        MAXIMUM_NUMBER_OF_STEPS = 2;
#endif
        
        this->loadGenericInfoFromDictionary(dict);
        
        this->setPosition(Point(0,0));
        this->setContentSize(prnt->getScene()->getContentSize());
        
        this->loadChildrenFromDictionary(dict);
        
        
        return true;
    }
    return false;
}

void LHGameWorldNode::onEnter(){

    //this needs to be called on onEnter or else when user uses transitions to move to a new scene scheduleUpdate will not work
    this->scheduleUpdate();
    
    Node::onEnter();
}

void LHGameWorldNode::update(float delta)
{
#if LH_USE_BOX2D
    this->step(delta);
#endif
}

#if COCOS2D_VERSION >= 0x00030200
void LHGameWorldNode::visit(Renderer *renderer, const Mat4& parentTransform, uint32_t parentFlags)
#else
void LHGameWorldNode::visit(Renderer *renderer, const Mat4& parentTransform, bool parentTransformUpdated)
#endif
{
    
    
    
#if LH_USE_BOX2D
#if LH_DEBUG
    _debugNode->clear();
    this->getBox2dWorld()->DrawDebugData();
#endif
#endif
    
    if(renderer)
    {
#if COCOS2D_VERSION >= 0x00030200
        Node::visit(renderer, parentTransform, parentFlags);
#else
        Node::visit(renderer, parentTransform, parentTransformUpdated);
#endif
    }
}

#pragma mark - BOX2D SUPPORT


#if LH_USE_BOX2D

b2World* LHGameWorldNode::getBox2dWorld()
{
    if(!_box2dWorld){
        
        b2Vec2 gravity;
        gravity.Set(0.f, 0.0f);
        _box2dWorld = new LHBox2dWorld(gravity, this->getScene());
//        _box2dWorld = new b2World(gravity);
        _box2dWorld->SetAllowSleeping(true);
        _box2dWorld->SetContinuousPhysics(true);

#if LH_DEBUG
        LHBox2dDebugDrawNode* drawNode = LHBox2dDebugDrawNode::create();
        drawNode->setLocalZOrder(99999);
        _box2dWorld->SetDebugDraw(drawNode->getDebug());
        drawNode->getDebug()->setRatio(((LHScene*)this->getScene())->getPtm());
        
        uint32 flags = 0;
        flags += b2Draw::e_shapeBit;
        flags += b2Draw::e_jointBit;
        //        //        flags += b2Draw::e_aabbBit;
        //        //        flags += b2Draw::e_pairBit;
        //        //        flags += b2Draw::e_centerOfMassBit;
        drawNode->getDebug()->SetFlags(flags);
        
        this->addChild(drawNode);
        _debugNode = drawNode;
#endif
        
    }
    return _box2dWorld;
}

void LHGameWorldNode::step(float dt)
{
    if(!this->getBox2dWorld())return;
    
	float32 frameTime = dt;
	int stepsPerformed = 0;
	while ( (frameTime > 0.0) && (stepsPerformed < MAXIMUM_NUMBER_OF_STEPS) ){
		float32 deltaTime = std::min( frameTime, FIXED_TIMESTEP );
		frameTime -= deltaTime;
		if (frameTime < MINIMUM_TIMESTEP) {
			deltaTime += frameTime;
			frameTime = 0.0f;
		}
		this->getBox2dWorld()->Step(deltaTime,VELOCITY_ITERATIONS,POSITION_ITERATIONS);
		stepsPerformed++;
        this->afterStep(dt); // process collisions and result from callbacks called by the step
	}
	this->getBox2dWorld()->ClearForces ();
}

void LHGameWorldNode::afterStep(float dt)
{
    for(size_t i = 0; i < _scheduledBeginContact.size(); ++i)
    {
        LHScheduledContactInfo info = _scheduledBeginContact[i];
        if(info.getNodeA() && info.getNodeB()){
            if(info.getNodeA()->getParent() && info.getNodeB()->getParent()){
                
                
                
                ((LHScene*)this->getScene())->didBeginContactBetweenNodes(info.getNodeA(),
                                                                          info.getNodeB(),
                                                                          info.getContactPoint(),
                                                                          info.getImpulse());
            }
        }
    }
    _scheduledBeginContact.clear();

    
    for(size_t i = 0; i < _scheduledEndContact.size(); ++i)
    {
        LHScheduledContactInfo info = _scheduledEndContact[i];
        if(info.getNodeA() && info.getNodeB()){
            if(info.getNodeA()->getParent() && info.getNodeB()->getParent()){
                ((LHScene*)this->getScene())->didEndContactBetweenNodes(info.getNodeA(),
                                                                        info.getNodeB());
            }
        }
    }
    _scheduledEndContact.clear();
}

void LHGameWorldNode::removeScheduledContactsWithNode(Node* node){
    
    std::vector<LHScheduledContactInfo>::iterator it;
    for(it = _scheduledBeginContact.begin(); it != _scheduledBeginContact.end();)
    {
        if(it->getNodeA() == node || it->getNodeB() == node)
        {
            it = _scheduledBeginContact.erase(it);
        }
        else{
            ++it;
        }
    }
    for(it = _scheduledEndContact.begin(); it != _scheduledEndContact.end();)
    {
        if(it->getNodeA() == node || it->getNodeB() == node)
        {
            it = _scheduledEndContact.erase(it);
        }
        else{
            ++it;
        }
    }
}

void LHGameWorldNode::scheduleDidBeginContactBetweenNodeA(Node* nodeA, Node* nodeB, Point contactPoint, float impulse)
{
    //this will restrict calling multiple contancts with same nodes (maybe the objects collide in two different points
//    for(size_t i = 0; i < _scheduledBeginContact.size(); ++i)
//    {
//        LHScheduledContactInfo info = _scheduledBeginContact[i];
//        if((info.getNodeA() == nodeA && info.getNodeB() == nodeB) ||
//           (info.getNodeA() == nodeB && info.getNodeB() == nodeA)
//           ){
//            return;
//        }
//    }
    
    //this means the object has already been removed but box2d still has it in the collision map
    if(nodeA->getParent() == NULL || nodeB->getParent() == NULL)return;
    
    _scheduledBeginContact.push_back(LHScheduledContactInfo(nodeA, nodeB, contactPoint, impulse));
}

void LHGameWorldNode::scheduleDidEndContactBetweenNodeA(Node* nodeA, Node* nodeB)
{
    //this will restrict calling multiple contancts with same nodes (maybe the objects collide in two different points
//    for(size_t i = 0; i < _scheduledEndContact.size(); ++i)
//    {
//        LHScheduledContactInfo info = _scheduledEndContact[i];
//        if((info.getNodeA() == nodeA && info.getNodeB() == nodeB) ||
//           (info.getNodeA() == nodeB && info.getNodeB() == nodeA)
//           ){
//            return;
//        }
//    }
    
    //this means the object has already been removed but box2d still has it in the collision map
    if(nodeA->getParent() == NULL || nodeB->getParent() == NULL)return;

    _scheduledEndContact.push_back(LHScheduledContactInfo(nodeA, nodeB, Point(), 0));
}

Point LHGameWorldNode::getGravity(){
    b2Vec2 grv = this->getBox2dWorld()->GetGravity();
    return Point(grv.x, grv.y);
}
void LHGameWorldNode::setGravity(Point gravity){
    b2Vec2 grv;
    grv.Set(gravity.x, gravity.y);
    this->getBox2dWorld()->SetGravity(grv);
}

#endif
