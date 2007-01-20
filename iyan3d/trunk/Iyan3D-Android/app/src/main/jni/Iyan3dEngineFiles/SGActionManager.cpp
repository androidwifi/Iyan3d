//
//  SGActionManager.cpp
//  Iyan3D
//
//  Created by Karthik on 26/12/15.
//  Copyright © 2015 Smackall Games. All rights reserved.
//

#include "HeaderFiles/SGActionManager.h"
#include "HeaderFiles/SGEditorScene.h"

SGEditorScene* actionScene;

SGActionManager::SGActionManager(SceneManager* smgr, void* scene)
{
    this->smgr = smgr;
    actionScene = (SGEditorScene*)scene;
    mirrorSwitchState = MIRROR_OFF;
    actions.clear();
    currentAction = 0;
}

SGActionManager::~SGActionManager()
{
    changeKeysAction.drop();
    assetAction.drop();
    propertyAction.drop();
    scaleAction.drop();
    addJointAction.drop();
    actions.clear();
}

void SGActionManager::addAction(SGAction& action){
    
    removeActions();
    
    actions.push_back(action);
    while(actions.size() > MAXUNDO){
        actions.erase(actions.begin());
        currentAction--;
    }
    currentAction++;
}

void SGActionManager::finalizeAndAddAction(SGAction& action)
{
    if(!actionScene || !smgr)
        return;
    
    if(action.actionType == ACTION_CHANGE_JOINT_KEYS|| action.actionType == ACTION_CHANGE_NODE_JOINT_KEYS|| action.actionType == ACTION_CHANGE_NODE_KEYS) {
        action.objectIndex = actionScene->nodes[actionScene->selectedNodeId]->actionId;
        if(action.actionType != ACTION_CHANGE_NODE_KEYS)
            action.actionSpecificIntegers.push_back(actionScene->selectedJointId);
        action.frameId = actionScene->currentFrame;
    }
    addAction(action);
}

void SGActionManager::removeActions()
{
    
    if(!actionScene || !smgr)
        return;

    if(currentAction < 0)
        currentAction = 0;
    
    while(actions.size()>currentAction)
        actions.pop_back();
}


bool SGActionManager::isIKJoint(int jointId)
{
    return (jointId == HAND_LEFT || jointId == HAND_RIGHT || jointId == LEG_LEFT || jointId == LEG_RIGHT || jointId == HIP || jointId == LEG_LEFT1 || jointId == LEG_RIGHT1);
}


bool SGActionManager::changeObjectOrientation(Vector3 outputValue)
{
    if(!actionScene || !smgr || !actionScene->isNodeSelected)
        return false;
    
    SGNode* selectedNode = actionScene->nodes[actionScene->selectedNodeId];
    
    bool success = false;
    switch(actionScene->controlType){
        case MOVE:{
            if(actionScene->isJointSelected){
                success = true;
                moveJoint(outputValue);
            }else if(actionScene->isNodeSelected){
                success = true;
                selectedNode->setPosition(selectedNode->node->getPosition() + outputValue, actionScene->currentFrame);
                selectedNode->setPositionOnNode(selectedNode->node->getPosition() + outputValue);
                break;
            }
            break;
        }
        case ROTATE:{
            outputValue*=RADTODEG;
            if(actionScene->isJointSelected){
                success = true;
                rotateJoint(outputValue);
             }else if(actionScene->isNodeSelected){
                success = true;
                Quaternion r = Quaternion(outputValue * DEGTORAD);
                selectedNode->setRotation(r, actionScene->currentFrame);
                selectedNode->setRotationOnNode(r);
                break;
            }
            break;
        }
        case SCALE: {
            break;
        }
        default:
            break;
    }
    actionScene->updater->updateControlsOrientaion();
    actionScene->updater->updateLightCamera();
    return success;
}

void SGActionManager::moveJoint(Vector3 outputValue, bool touchMove)
{
    
    SGNode* selectedNode = actionScene->nodes[actionScene->selectedNodeId];
    SGJoint * selectedJoint = actionScene->selectedJoint;
    
    Vector3 target = (touchMove) ? outputValue : selectedJoint->jointNode->getAbsolutePosition() + outputValue;
    if((selectedNode->joints.size() == HUMAN_JOINTS_SIZE) && isIKJoint(actionScene->selectedJointId)){
        if(actionScene->selectedJointId <= HIP){
            shared_ptr<AnimatedMeshNode> rigNode = dynamic_pointer_cast<AnimatedMeshNode>(selectedNode->node);
            if(rigNode) {
                Vector3 newPosition = (touchMove) ? outputValue : rigNode->getPosition() + outputValue;
                selectedNode->setPosition(newPosition, actionScene->currentFrame);
                selectedNode->setPositionOnNode(newPosition);
                rigNode->setPosition(newPosition);
                rigNode->updateAbsoluteTransformation();
                for (actionScene->ikJointsPositoinMapItr = actionScene->ikJointsPositionMap.begin(); actionScene->ikJointsPositoinMapItr != actionScene->ikJointsPositionMap.end(); actionScene->ikJointsPositoinMapItr++){
                    int jointId = ((*actionScene->ikJointsPositoinMapItr).first);
                    selectedNode->MoveBone((shared_ptr<JointNode>)(selectedNode->joints[jointId]->jointNode),actionScene->ikJointsPositionMap.find((*actionScene->ikJointsPositoinMapItr).first)->second,actionScene->currentFrame);
                }
            }
        }else{
            if(selectedJoint)
                selectedNode->MoveBone(selectedJoint->jointNode,target,actionScene->currentFrame);
        }
    } else if(selectedNode->getType() == NODE_TEXT){
        
        selectedJoint = selectedNode->joints[actionScene->selectedJointId];
        Vector3 jointLocalPos = selectedJoint->jointNode->getPosition();
        
        Vector3 moveDir = Vector3((actionScene->selectedControlId == X_MOVE ? 1.0 : 0.0),(actionScene->selectedControlId == Y_MOVE ? 1.0 : 0.0),(actionScene->selectedControlId == Z_MOVE ? 1.0 : 0.0));
        
        float trnsValue = (actionScene->selectedControlId == X_MOVE) ? outputValue.x : (actionScene->selectedControlId == Y_MOVE) ? outputValue.y : outputValue.z;
        moveDir = moveDir * trnsValue;
        
        Vector3 position = jointLocalPos + moveDir;
        selectedJoint->setPosition(position, actionScene->currentFrame);
        selectedJoint->setPositionOnNode(position);
        
    }
    else{
        shared_ptr<Node> parent = selectedJoint->jointNode->getParent();
        Vector3 jointPos = selectedJoint->jointNode->getAbsoluteTransformation().getTranslation();
        Quaternion rot = Quaternion(MathHelper::getRelativeParentRotation(selectedJoint->jointNode,(jointPos + outputValue)) * DEGTORAD);
        selectedNode->joints[parent->getID()]->setRotation(rot, actionScene->currentFrame);
        selectedNode->joints[parent->getID()]->setRotationOnNode(rot);
        
        if(actionScene->getMirrorState() == MIRROR_ON){
            int mirrorJointId = BoneLimitsHelper::getMirrorJointId(actionScene->selectedJointId);
            if(mirrorJointId != -1) {
                selectedJoint = selectedNode->joints[actionScene->selectedJointId];
                shared_ptr<JointNode> mirrorNode = (dynamic_pointer_cast<AnimatedMeshNode>(selectedNode->node))->getJointNode(mirrorJointId);
                rot = Quaternion(selectedNode->joints[selectedJoint->jointNode->getParent()->getID()]->jointNode->getRotationInDegrees()*Vector3(1.0,-1.0,-1.0)*DEGTORAD);
                selectedNode->joints[mirrorNode->getParent()->getID()]->setRotation(rot,actionScene->currentFrame);
                selectedNode->joints[mirrorNode->getParent()->getID()]->setRotationOnNode(rot);
            }
        }
    }
// TODO For CPU Skin update mesh cache
}

void SGActionManager::rotateJoint(Vector3 outputValue)
{
    SGNode* selectedNode = actionScene->nodes[actionScene->selectedNodeId];
    SGJoint * selectedJoint = actionScene->selectedJoint;

    selectedJoint->setRotation(Quaternion(outputValue*DEGTORAD), actionScene->currentFrame);
    selectedJoint->setRotationOnNode(Quaternion(outputValue*DEGTORAD));
    if(actionScene->getMirrorState() == MIRROR_ON){
        int mirrorJointId = BoneLimitsHelper::getMirrorJointId(actionScene->selectedJointId);
        if(mirrorJointId != -1) {
            Quaternion rot = Quaternion(outputValue*Vector3(1.0,-1.0,-1.0)*DEGTORAD);
            selectedNode->joints[mirrorJointId]->setRotation(rot, actionScene->currentFrame);
            selectedNode->joints[mirrorJointId]->setRotationOnNode(rot);
        }
    }
    // TODO For CPU Skin update mesh cache
}
void SGActionManager::storeActionKeys(bool finished)
{
    if(!actionScene || !smgr || !actionScene->isNodeSelected)
        return;

    SGNode * selectedNode = actionScene->nodes[actionScene->selectedNodeId];
    if(actionScene->selectedControlId != NOT_SELECTED) {
        actionScene->isControlSelected = true;
        if(!finished)
            changeKeysAction.drop();
        changeKeysAction.objectIndex = selectedNode->actionId;
        
        if(!actionScene->isJointSelected){
            changeKeysAction.actionType = ACTION_CHANGE_NODE_KEYS;
            changeKeysAction.keys.push_back(selectedNode->getKeyForFrame(actionScene->currentFrame));
            if(finished) {
                if(selectedNode->getType() == NODE_LIGHT || selectedNode->getType() == NODE_ADDITIONAL_LIGHT)
                    actionScene->updater->updateLightProperties(actionScene->currentFrame);
            }
        }
        else if(actionScene->isJointSelected){
            if(selectedNode->joints.size() >= HUMAN_JOINTS_SIZE && actionScene->selectedJointId == HIP) {
                changeKeysAction.actionType = ACTION_CHANGE_NODE_JOINT_KEYS;
                changeKeysAction.keys.push_back(selectedNode->getKeyForFrame(actionScene->currentFrame));
                for(unsigned long i=0; i<selectedNode->joints.size(); i++){
                    changeKeysAction.keys.push_back(selectedNode->joints[i]->getKeyForFrame(actionScene->currentFrame));
                }
            }
            else {
                changeKeysAction.actionType = ACTION_CHANGE_JOINT_KEYS;
                for(unsigned long i=0; i<selectedNode->joints.size(); i++){
                    changeKeysAction.keys.push_back(selectedNode->joints[i]->getKeyForFrame(actionScene->currentFrame));
                }
            }
        }
        
        actionScene->updater->reloadKeyFrameMap();
        if(finished)
            finalizeAndAddAction(changeKeysAction);
    }
}


void SGActionManager::switchFrame(int frame)
{
    if(!actionScene || !smgr)
        return;

    if(!actionScene->isPlaying) {
        SGAction switchFrameAction;
        switchFrameAction.actionType = ACTION_SWITCH_FRAME;
        switchFrameAction.actionSpecificIntegers.push_back(actionScene->previousFrame);
        switchFrameAction.frameId = actionScene->currentFrame;
        addAction(switchFrameAction);
    }
    actionScene->updater->setDataForFrame(frame);
}

void SGActionManager::changeMeshProperty(float brightness, float specular, bool isLighting, bool isVisible, bool isChanged)
{
    if(!actionScene || !smgr || !actionScene->isNodeSelected)
        return;
    
    SGNode *selectedNode = actionScene->nodes[actionScene->selectedNodeId];
    
    if(propertyAction.actionType == ACTION_EMPTY){
        propertyAction.actionType = ACTION_CHANGE_PROPERTY_MESH;
        propertyAction.objectIndex = selectedNode->actionId;
        propertyAction.frameId = actionScene->currentFrame;
        propertyAction.actionSpecificFloats.push_back(selectedNode->props.brightness);
        propertyAction.actionSpecificFloats.push_back(selectedNode->props.shininess);
        propertyAction.actionSpecificFlags.push_back(selectedNode->props.isLighting);
        propertyAction.actionSpecificFlags.push_back(selectedNode->props.isVisible);
    }
    
    selectedNode->setShaderProperties(brightness, specular, isLighting, isVisible, actionScene->currentFrame);
    if(isChanged){
        propertyAction.actionSpecificFloats.push_back(brightness);
        propertyAction.actionSpecificFloats.push_back(specular);
        propertyAction.actionSpecificFlags.push_back(isLighting);
        propertyAction.actionSpecificFlags.push_back(isVisible);
        finalizeAndAddAction(propertyAction);
        propertyAction.drop();
    }
    actionScene->updater->setDataForFrame(actionScene->currentFrame);
}

void SGActionManager::changeCameraProperty(float fov , int resolutionType, bool isChanged)
{
    if(!actionScene || !smgr || !actionScene->isNodeSelected)
        return;

    if(propertyAction.actionType == ACTION_EMPTY){
        propertyAction.actionType = ACTION_CHANGE_PROPERTY_CAMERA;
        propertyAction.actionSpecificFloats.push_back(actionScene->cameraFOV);
        propertyAction.actionSpecificIntegers.push_back(actionScene->cameraResolutionType);
    }
    actionScene->updater->setCameraProperty(fov, resolutionType);
    if(isChanged){
        propertyAction.actionSpecificFloats.push_back(actionScene->cameraFOV);
        propertyAction.actionSpecificIntegers.push_back(actionScene->cameraResolutionType);
        finalizeAndAddAction(propertyAction);
        propertyAction.drop();
    }
    
}

void SGActionManager::changeLightProperty(float red , float green, float blue, float shadow, bool isChanged)
{
    if(!actionScene || !smgr || actionScene->selectedNodeId == NOT_EXISTS)
        return;
    
    SGNode *selectedNode = actionScene->nodes[actionScene->selectedNodeId];
    if(propertyAction.actionType == ACTION_EMPTY){
        propertyAction.actionType = ACTION_CHANGE_PROPERTY_LIGHT;
        propertyAction.actionSpecificFloats.push_back(ShaderManager::lightColor[0].x);
        propertyAction.actionSpecificFloats.push_back(ShaderManager::lightColor[0].y);
        propertyAction.actionSpecificFloats.push_back(ShaderManager::lightColor[0].z);
        propertyAction.actionSpecificFloats.push_back(ShaderManager::shadowDensity);
        
        changeKeysAction.drop();
        changeKeysAction.actionType = ACTION_CHANGE_NODE_KEYS;
        changeKeysAction.keys.push_back(selectedNode->getKeyForFrame(actionScene->currentFrame));
    }
    
    if(selectedNode->getType() == NODE_LIGHT){
        ShaderManager::shadowDensity = shadow;
    }
    else if(selectedNode->getType() == NODE_ADDITIONAL_LIGHT) {
        selectedNode->props.nodeSpecificFloat = (shadow + 0.001) * 300.0;
    }
    
    //nodes[selectedNodeId]->props.vertexColor = Vector3(red,green,blue);
    Quaternion lightPropKey = Quaternion(red,green,blue,selectedNode->props.nodeSpecificFloat);
    Vector3 mainLightColor = Vector3(red,green,blue);
    
    if(selectedNode->getType() == NODE_LIGHT)
        selectedNode->setScale(mainLightColor, actionScene->currentFrame);
    else
        selectedNode->setRotation(lightPropKey, actionScene->currentFrame);
    
    actionScene->updater->updateLightProperties(actionScene->currentFrame);
    //    updateLightWithRender();
}

void SGActionManager::storeLightPropertyChangeAction(float red , float green , float blue , float shadowDensity)
{
    if(!actionScene || !smgr || actionScene->selectedNodeId != NOT_EXISTS)
        return;
    SGNode* selectedNode = actionScene->nodes[actionScene->selectedNodeId];
    changeKeysAction.keys.push_back(selectedNode->getKeyForFrame(actionScene->currentFrame));
    
    propertyAction.actionSpecificFloats.push_back(red);
    propertyAction.actionSpecificFloats.push_back(green);
    propertyAction.actionSpecificFloats.push_back(blue);
    propertyAction.actionSpecificFloats.push_back(shadowDensity);
    finalizeAndAddAction(changeKeysAction);
    changeKeysAction.drop();
    propertyAction.drop();
}

void SGActionManager::setMirrorState(MIRROR_SWITCH_STATE flag)
{
    if(!actionScene || !smgr || actionScene->selectedNodeId != NOT_EXISTS)
        return;

    mirrorSwitchState = flag;
    if(actionScene->isJointSelected)
        actionScene->selectMan->highlightJointSpheres();
}
bool SGActionManager::switchMirrorState()
{
    if(!actionScene || !smgr || actionScene->selectedNodeId != NOT_EXISTS)
        return false;

    SGAction action;
    action.actionType = ACTION_CHANGE_MIRROR_STATE;
    action.actionSpecificFlags.push_back(mirrorSwitchState);
    finalizeAndAddAction(action);
    setMirrorState((MIRROR_SWITCH_STATE)!mirrorSwitchState);
    return mirrorSwitchState;
}

MIRROR_SWITCH_STATE SGActionManager::getMirrorState()
{
    return mirrorSwitchState;
}

void SGActionManager::storeAddOrRemoveAssetAction(int actionType, int assetId, string optionalFilePath)
{
    if(!actionScene || !smgr)
        return;

    if (actionType == ACTION_NODE_ADDED) {
        assetAction.drop();
        assetAction.actionType = ACTION_NODE_ADDED;
        assetAction.frameId = assetId;
        assetAction.objectIndex =  actionScene->actionObjectsSize;
        addAction(assetAction);

    } else if(actionType == ACTION_NODE_DELETED) {
        assetAction.drop();
        assetAction.actionType = ACTION_NODE_DELETED;
        SGNode * selectedNode = actionScene->nodes[actionScene->selectedNodeId];
        assetAction.frameId = selectedNode->assetId;
        assetAction.objectIndex = selectedNode->actionId;
        assetAction.actionSpecificStrings.push_back(ConversionHelper::getWStringForString(selectedNode->props.prevMatName));
        StoreDeleteObjectKeys(actionScene->selectedNodeId);
        addAction(assetAction);
    } else if (actionType == ACTION_TEXT_IMAGE_DELETE|| actionType == ACTION_TEXT_IMAGE_ADD) {
        assetAction.drop();
        assetAction.actionType = actionType == ACTION_TEXT_IMAGE_DELETE?ACTION_TEXT_IMAGE_DELETE : ACTION_TEXT_IMAGE_ADD;
        int indexOfAsset = actionType == ACTION_TEXT_IMAGE_DELETE ? actionScene->selectedNodeId : (int)actionScene->nodes.size()-1;
        assetAction.objectIndex = actionScene->nodes[indexOfAsset]->actionId;
        assetAction.actionSpecificStrings.push_back(ConversionHelper::getWStringForString(actionScene->nodes[indexOfAsset]->node->material->name));
        assetAction.actionSpecificStrings.push_back(ConversionHelper::getWStringForString(actionScene->nodes[indexOfAsset]->optionalFilePath));
        assetAction.actionSpecificStrings.push_back(actionScene->nodes[indexOfAsset]->name);
        assetAction.actionSpecificFloats.push_back(actionScene->nodes[indexOfAsset]->props.vertexColor.x);
        assetAction.actionSpecificFloats.push_back(actionScene->nodes[indexOfAsset]->props.vertexColor.y);
        assetAction.actionSpecificFloats.push_back(actionScene->nodes[indexOfAsset]->props.vertexColor.z);
        assetAction.actionSpecificFloats.push_back(actionScene->nodes[indexOfAsset]->props.nodeSpecificFloat);
        assetAction.actionSpecificIntegers.push_back(actionScene->nodes[indexOfAsset]->getType());
        assetAction.actionSpecificIntegers.push_back(actionScene->nodes[indexOfAsset]->props.fontSize);
        if(actionType == ACTION_TEXT_IMAGE_DELETE)
            StoreDeleteObjectKeys(indexOfAsset);
        addAction(assetAction);
    } else if (actionType == ACTION_APPLY_ANIM) {
        assetAction.drop();
        assetAction.actionType = ACTION_APPLY_ANIM;
        assetAction.actionSpecificStrings.push_back(ConversionHelper::getWStringForString(actionScene->animMan->animFilePath));
        assetAction.objectIndex = actionScene->nodes[actionScene->selectedNodeId]->actionId;
        assetAction.frameId = actionScene->animMan->animStartFrame;
        assetAction.actionSpecificIntegers.push_back(actionScene->animMan->animTotalFrames);
        addAction(assetAction);
    }
}

void SGActionManager::StoreDeleteObjectKeys(int nodeIndex)
{
    if(!actionScene || !smgr)
        return;
    
    SGNode *sgNode = actionScene->nodes[nodeIndex];

    if(sgNode->positionKeys.size())
        assetAction.nodePositionKeys = sgNode->positionKeys;
    if(sgNode->rotationKeys.size())
        assetAction.nodeRotationKeys = sgNode->rotationKeys;
    if(sgNode->scaleKeys.size())
        assetAction.nodeSCaleKeys = sgNode->scaleKeys;
    if(sgNode->visibilityKeys.size())
        assetAction.nodeVisibilityKeys = sgNode->visibilityKeys;
    for (int i = 0; i < (int)sgNode->joints.size(); i++) {
        //assetAction.jointsRotationKeys[i] = nodes[nodeIndex]->joints[i]->rotationKeys;
        assetAction.jointRotKeys[i] = sgNode->joints[i]->rotationKeys;
        if(sgNode->getType() == NODE_TEXT){
            assetAction.jointPosKeys[i] = sgNode->joints[i]->positionKeys;
            assetAction.jointScaleKeys[i] = sgNode->joints[i]->scaleKeys;
        }
    }
}

void SGActionManager::changeObjectScale(Vector3 scale, bool isChanged)
{
    if((actionScene->isJointSelected  && actionScene->nodes[actionScene->selectedNodeId]->getType() != NODE_TEXT) || !actionScene->isNodeSelected) return;
    
    if(actionScene->actionMan->scaleAction.actionType == ACTION_EMPTY){
        if(actionScene->isJointSelected && actionScene->nodes[actionScene->selectedNodeId]->getType() == NODE_TEXT) {
            scaleAction.actionType = ACTION_CHANGE_JOINT_KEYS;
            scaleAction.objectIndex =actionScene->nodes[actionScene->selectedNodeId]->actionId;
            for (int i = 0; i < actionScene->nodes[actionScene->selectedNodeId]->joints.size(); i++)
                scaleAction.keys.push_back(actionScene->nodes[actionScene->selectedNodeId]->joints[i]->getKeyForFrame(actionScene->currentFrame));
        } else {
            scaleAction.actionType = ACTION_CHANGE_NODE_KEYS;
            scaleAction.objectIndex = actionScene->nodes[actionScene->selectedNodeId]->actionId;
            scaleAction.keys.push_back(actionScene->nodes[actionScene->selectedNodeId]->getKeyForFrame(actionScene->currentFrame));
        }
    }
    if(isChanged){
        if(actionScene->isJointSelected && actionScene->nodes[actionScene->selectedNodeId]->getType() == NODE_TEXT) {
            for (int i = 0; i < actionScene->nodes[actionScene->selectedNodeId]->joints.size(); i++)
                scaleAction.keys.push_back(actionScene->nodes[actionScene->selectedNodeId]->joints[i]->getKeyForFrame(actionScene->currentFrame));
        } else {
            scaleAction.keys.push_back(actionScene->nodes[actionScene->selectedNodeId]->getKeyForFrame(actionScene->currentFrame));
        }
        addAction(scaleAction);
        scaleAction.drop();
    }
    if(actionScene->isJointSelected && actionScene->nodes[actionScene->selectedNodeId]->getType() == NODE_TEXT){
        actionScene->nodes[actionScene->selectedNodeId]->joints[actionScene->selectedJointId]->setScale(scale, actionScene->currentFrame);
    } else {
        actionScene->nodes[actionScene->selectedNodeId]->setScale(scale, actionScene->currentFrame);
    }
    actionScene->updater->setDataForFrame(actionScene->currentFrame);
    actionScene->updater->reloadKeyFrameMap();
    actionScene->updater->updateControlsOrientaion();
}

void SGActionManager::removeDemoAnimation()
{
    SGAction &recentAction = actions[currentAction-1];
    int objectIndex = recentAction.objectIndex;
        int indexOfAction = getObjectIndex(objectIndex);
        actionScene->selectedNodeId = indexOfAction;
        actionScene->animMan->removeAppliedAnimation(recentAction.frameId, recentAction.actionSpecificIntegers[0]);
        actions.erase(actions.begin()+(currentAction-1));
        currentAction--;
    
}

int SGActionManager::undo(int &returnValue2)
{
    if(!actionScene || !smgr)
        return -1;

    int returnValue = DO_NOTHING;
    
    SGAction &recentAction = actions[currentAction-1];
    int indexOfAction = 0;
    
    if(currentAction <= 0) {
        currentAction = 0;
        return (actions.size() > 0) ? DEACTIVATE_UNDO:DEACTIVATE_BOTH;
    }
    returnValue2 = indexOfAction = getObjectIndex(recentAction.objectIndex);
    
    switch(recentAction.actionType){
        case ACTION_CHANGE_NODE_KEYS:
            actionScene->nodes[indexOfAction]->setKeyForFrame(recentAction.frameId, recentAction.keys[0]);
            actionScene->updater->reloadKeyFrameMap();
            break;
        case ACTION_CHANGE_JOINT_KEYS: {
            for(int i = 0; i <(int)actionScene->nodes[indexOfAction]->joints.size(); i++){
                actionScene->nodes[indexOfAction]->joints[i]->setKeyForFrame(recentAction.frameId, recentAction.keys[i]);
            }
            actionScene->updater->reloadKeyFrameMap();
            break;
            
        }
        case ACTION_CHANGE_NODE_JOINT_KEYS:{
            actionScene->nodes[indexOfAction]->setKeyForFrame(recentAction.frameId, recentAction.keys[0]);
            for(unsigned long i=0; i < actionScene->nodes[indexOfAction]->joints.size(); i++){
                actionScene->nodes[indexOfAction]->joints[i]->setKeyForFrame(recentAction.frameId, recentAction.keys[i+1]);
            }
            actionScene->updater->reloadKeyFrameMap();
            break;
        }
        case ACTION_SWITCH_FRAME:{
            actionScene->currentFrame = recentAction.actionSpecificIntegers[0];
            returnValue = SWITCH_FRAME;
            break;
        }
        case ACTION_CHANGE_PROPERTY_MESH:{
            actionScene->nodes[indexOfAction]->setShaderProperties(recentAction.actionSpecificFloats[0], recentAction.actionSpecificFloats[1], recentAction.actionSpecificFlags[0], recentAction.actionSpecificFlags[1], recentAction.frameId);
            break;
        }
        case ACTION_CHANGE_PROPERTY_LIGHT: {
            //TODO to do for all lights
            ShaderManager::lightColor[0] = Vector3(recentAction.actionSpecificFloats[0],recentAction.actionSpecificFloats[1],recentAction.actionSpecificFloats[2]);
            ShaderManager::shadowDensity = recentAction.actionSpecificFloats[3];
            break;
        }
        case ACTION_CHANGE_PROPERTY_CAMERA:{
            actionScene->updater->setCameraProperty(recentAction.actionSpecificFloats[0], recentAction.actionSpecificIntegers[0]);
            break;
        }
        case ACTION_CHANGE_MIRROR_STATE:{
            actionScene->selectMan->unselectObject(actionScene->selectedNodeId);
            setMirrorState((MIRROR_SWITCH_STATE)(bool)recentAction.actionSpecificFlags[0]);
            returnValue = SWITCH_MIRROR;
            break;
        }
        case ACTION_NODE_ADDED: {
            recentAction.actionSpecificStrings.push_back(ConversionHelper::getWStringForString(actionScene->nodes[indexOfAction]->props.prevMatName));
            actionScene->selectMan->unselectObject(actionScene->selectedNodeId);
            returnValue = DELETE_ASSET;
            break;
        }
        case ACTION_NODE_DELETED: {
            returnValue = ADD_ASSET_BACK;
            returnValue2 = recentAction.frameId;
            break;
        }
        case ACTION_TEXT_IMAGE_ADD: {
            actionScene->selectMan->unselectObject(actionScene->selectedNodeId);
            returnValue = DELETE_ASSET;
            break;
        }
        case ACTION_TEXT_IMAGE_DELETE: {
            if(!actionScene->loader->loadNodeOnUndoORedo(recentAction, UNDO_ACTION)){
                returnValue = DO_NOTHING;
                break;
            }
            returnValue = ADD_TEXT_IMAGE_BACK;
            break;
        }
        case ACTION_APPLY_ANIM: {
            actionScene->selectMan->unselectObject(actionScene->selectedNodeId);
            actionScene->selectedNodeId = indexOfAction;
            actionScene->animMan->removeAppliedAnimation(recentAction.frameId, recentAction.actionSpecificIntegers[0]);
            returnValue = RELOAD_FRAMES;
            break;
        }
        default:
            return DO_NOTHING;
    }
    if(recentAction.actionType != ACTION_NODE_DELETED && recentAction.actionType != ACTION_TEXT_IMAGE_DELETE)
        currentAction--;
    
    if(recentAction.actionType != ACTION_SWITCH_FRAME) {
        actionScene->updater->setKeysForFrame(recentAction.frameId);
        actionScene->updater->setKeysForFrame(recentAction.frameId);
    }
        // CPU SKIN update
    
    if(recentAction.actionType == ACTION_CHANGE_NODE_KEYS && (actionScene->nodes[indexOfAction]->getType() == NODE_LIGHT || actionScene->nodes[indexOfAction]->getType() == NODE_ADDITIONAL_LIGHT))
        actionScene->updater->updateLightProperties(recentAction.frameId);
    
    return returnValue;

}

int SGActionManager::redo()
{
    if(!actionScene || !smgr)
        return -1;

    int returnValue = DO_NOTHING;
    
    if(currentAction == actions.size()) {
        if(currentAction <= 0) {
            currentAction = 0;
            return DEACTIVATE_BOTH;
        }
        else
            return DEACTIVATE_REDO;
    }
    

    
    SGAction &recentAction = actions[currentAction];
    int indexOfAction = getObjectIndex(recentAction.objectIndex);
    
    SGNode* sgNode;
    if(indexOfAction < actionScene->nodes.size())
        sgNode = actionScene->nodes[indexOfAction];

    
    switch(recentAction.actionType){
        case ACTION_CHANGE_NODE_KEYS:{
            sgNode->setKeyForFrame(recentAction.frameId, recentAction.keys[1]);
            actionScene->updater->reloadKeyFrameMap();
        }
            break;
        case ACTION_CHANGE_JOINT_KEYS:{
            int jointsCnt = (int)sgNode->joints.size();
            for(unsigned long i=0; i < jointsCnt; i++){
                sgNode->joints[i]->setKeyForFrame(recentAction.frameId, recentAction.keys[jointsCnt+i]);
            }
            actionScene->updater->reloadKeyFrameMap();
            break;
        }
        case ACTION_CHANGE_NODE_JOINT_KEYS:{
            int jointsCnt = (int)sgNode->joints.size();
            sgNode->setKeyForFrame(recentAction.frameId, recentAction.keys[jointsCnt +1]);
            for(unsigned long i=1; i <= sgNode->joints.size(); i++){
                sgNode->joints[i-1]->setKeyForFrame(recentAction.frameId, recentAction.keys[jointsCnt+i+1]);
            }
            break;
        }
        case ACTION_SWITCH_FRAME:
            actionScene->currentFrame = recentAction.frameId;
            break;
        case ACTION_CHANGE_PROPERTY_MESH:
            sgNode->setShaderProperties(recentAction.actionSpecificFloats[2], recentAction.actionSpecificFloats[3], recentAction.actionSpecificFlags[2], recentAction.actionSpecificFlags[3], recentAction.frameId);
            break;
        case ACTION_CHANGE_PROPERTY_LIGHT:
            ShaderManager::lightColor[0] = Vector3(recentAction.actionSpecificFloats[4],recentAction.actionSpecificFloats[5],recentAction.actionSpecificFloats[6]);
            ShaderManager::shadowDensity = recentAction.actionSpecificFloats[7];
            break;
        case ACTION_CHANGE_PROPERTY_CAMERA:
            actionScene->updater->setCameraProperty(recentAction.actionSpecificFloats[1], recentAction.actionSpecificIntegers[1]);
            break;
        case ACTION_CHANGE_MIRROR_STATE:
            actionScene->selectMan->unselectObject(actionScene->selectedNodeId);
            setMirrorState((MIRROR_SWITCH_STATE)!recentAction.actionSpecificFlags[0]);
            returnValue = SWITCH_MIRROR;
            break;
        case ACTION_NODE_ADDED:
        {
            returnValue = recentAction.frameId;
            break;
        }
        case ACTION_NODE_DELETED:
            actionScene->selectMan->unselectObject(actionScene->selectedNodeId);
            actionScene->selectedNodeId = indexOfAction;
            returnValue = DELETE_ASSET;
            break;
        case ACTION_TEXT_IMAGE_ADD: {
            if(!actionScene->loader->loadNodeOnUndoORedo(recentAction, REDO_ACTION)) {
                returnValue = DO_NOTHING;
                break;
            }
            returnValue = ADD_TEXT_IMAGE_BACK;
            break;
        }
        case ACTION_TEXT_IMAGE_DELETE: {
            actionScene->selectMan->unselectObject(actionScene->selectedNodeId);
            actionScene->selectedNodeId = indexOfAction;
            returnValue = DELETE_ASSET;
            break;
        }
        case ACTION_APPLY_ANIM: {
            actionScene->selectedNodeId = indexOfAction;
            actionScene->updater->setDataForFrame(actionScene->currentFrame);
            actionScene->animMan->applyAnimations(ConversionHelper::getStringForWString(recentAction.actionSpecificStrings[0]), actionScene->selectedNodeId);
            actionScene->updater->reloadKeyFrameMap();
            actionScene->selectMan->unselectObject(actionScene->selectedNodeId);
            break;
        }
        default:
            return DO_NOTHING;
    }
    if(recentAction.actionType != ACTION_NODE_ADDED && recentAction.actionType != ACTION_TEXT_IMAGE_ADD)
        currentAction++;
    if(recentAction.actionType != ACTION_SWITCH_FRAME)
        actionScene->updater->setKeysForFrame(actionScene->currentFrame);
    actionScene->updater->updateControlsOrientaion();
    
    if(recentAction.actionType == ACTION_CHANGE_NODE_KEYS && (sgNode->getType() == NODE_LIGHT || sgNode->getType() == NODE_ADDITIONAL_LIGHT))
        actionScene->updater->updateLightProperties(recentAction.frameId);
    
    return returnValue;
}

int SGActionManager::getObjectIndex(int actionIndex)
{
        for(int i = 0; i < (int)actionScene->nodes.size(); i++) {
        if(actionScene->nodes[i]->actionId == actionIndex) {
            return i;
        }
    }
    return 0;
}

bool SGActionManager::changeSkeletonPosition(Vector3 outputValue)
{
    if(!actionScene || !smgr || !actionScene->isRigMode)
        return false;
    
    bool isNodeSelected = actionScene->hasNodeSelected();
    int selectedNodeId = actionScene->rigMan->selectedNodeId;
    SGNode* selectedNode = actionScene->getSelectedNode();
    std::map<int, RigKey>& rigKeys = actionScene->rigMan->rigKeys;
    
    if(!isNodeSelected || actionScene->rigMan->sceneMode == RIG_MODE_EDIT_ENVELOPES || actionScene->rigMan->sceneMode == RIG_MODE_PREVIEW)
        return false;
    
    if(selectedNodeId > 0){
        Mat4 parentGlobalMat = selectedNode->node->getParent()->getAbsoluteTransformation();
        selectedNode->node->setPosition(MathHelper::getRelativePosition(parentGlobalMat, selectedNode->node->getAbsolutePosition() + outputValue));
        selectedNode->node->updateAbsoluteTransformation();
        actionScene->updater->updateSkeletonBone(rigKeys, selectedNodeId);
        
        if(getMirrorState() == MIRROR_ON){
            int mirrorJointId = BoneLimitsHelper::getMirrorJointId(selectedNodeId);
            if(mirrorJointId != -1){
                {
                    shared_ptr<Node> mirrorNode = rigKeys[mirrorJointId].referenceNode->node;
                    mirrorNode->setPosition(selectedNode->node->getPosition() * Vector3(-1.0,1.0,1.0));
                    mirrorNode->updateAbsoluteTransformation();
                    actionScene->updater->updateSkeletonBone(rigKeys, mirrorJointId);
                }
            }
        }
        actionScene->updater->updateSkeletonBones();
        return true;
    }
    else if(selectedNodeId == 0){
        selectedNode->node->setPosition(selectedNode->node->getAbsolutePosition() + outputValue);
        selectedNode->node->updateAbsoluteTransformation();
        return true;
    }
    return false;
}

bool SGActionManager::changeSkeletonRotation(Vector3 outputValue)
{
    if(!actionScene || !smgr || !actionScene->isRigMode || actionScene->rigMan->sceneMode != RIG_MODE_MOVE_JOINTS)
        return false;

    outputValue *= RADTODEG;
    
    bool isNodeSelected = actionScene->hasNodeSelected();
    int selectedNodeId = actionScene->rigMan->selectedNodeId;
    SGNode* selectedNode = actionScene->getSelectedNode();
    
    if (actionScene->rigMan->sceneMode == RIG_MODE_MOVE_JOINTS && selectedNodeId > 0){
        selectedNode->node->setRotationInDegrees(outputValue);
        selectedNode->node->updateAbsoluteTransformation();
        actionScene->updater->updateSkeletonBone(actionScene->rigMan->rigKeys, selectedNodeId);
        if(getMirrorState() == MIRROR_ON)
        {
            int mirrorJointId = BoneLimitsHelper::getMirrorJointId(selectedNodeId);
            if(mirrorJointId != -1){
                shared_ptr<Node> mirrorNode = actionScene->rigMan->rigKeys[mirrorJointId].referenceNode->node;
                mirrorNode->setRotationInDegrees(outputValue * Vector3(1.0,-1.0,-1.0));
                mirrorNode->updateAbsoluteTransformation();
                actionScene->updater->updateSkeletonBone(actionScene->rigMan->rigKeys, mirrorJointId);
                //mirrorNode.reset();
            }
        }
        //        updateEnvelopes();
        return true;
    }
    else if(selectedNode){
        selectedNode->node->setRotationInDegrees(outputValue);
        if(isNodeSelected)
            selectedNode->node->updateAbsoluteTransformationOfChildren();
        else selectedNode->node->updateAbsoluteTransformation();
        return true;
    }
    return false;
}

bool SGActionManager::changeSGRPosition(Vector3 outputValue)
{
    if(!actionScene || !smgr || !actionScene->isRigMode || actionScene->rigMan->sceneMode != RIG_MODE_PREVIEW)
        return false;

    bool isJointSelected = actionScene->hasJointSelected();
    bool isNodeSelected = actionScene->hasNodeSelected();
    SGNode* selectedNode = actionScene->getSelectedNode();
    SGJoint* selectedJoint = actionScene->getSelectedJoint();
    int selectedJointId = actionScene->rigMan->selectedJointId;
    SGNode* sgrSGNode = actionScene->rigMan->getRiggedNode();
    
    if(isJointSelected){
        Vector3 target = selectedJoint->jointNode->getAbsolutePosition() + outputValue;
        selectedJoint->jointNode->getParent()->setRotationInDegrees(MathHelper::getRelativeParentRotation(selectedJoint->jointNode,target));
        (dynamic_pointer_cast<JointNode>(selectedJoint->jointNode->getParent()))->updateAbsoluteTransformationOfChildren();
        if(getMirrorState() == MIRROR_ON){
            int mirrorJointId = BoneLimitsHelper::getMirrorJointId(selectedJointId);
            if(mirrorJointId != -1){
                shared_ptr<JointNode> mirrorNode = (dynamic_pointer_cast<AnimatedMeshNode>(sgrSGNode->node))->getJointNode(mirrorJointId);
                mirrorNode->getParent()->setRotationInDegrees(selectedJoint->jointNode->getParent()->getRotationInDegrees() * Vector3(1.0,-1.0,-1.0));
                (dynamic_pointer_cast<JointNode>(mirrorNode->getParent()))->updateAbsoluteTransformationOfChildren();
            }
        }
        return true;
    }
    else if(isNodeSelected){
        selectedNode->node->setPosition(selectedNode->node->getAbsolutePosition() + outputValue);
        selectedNode->node->updateAbsoluteTransformationOfChildren();
        return true;
    }
    return false;
}

bool SGActionManager::changeSGRRotation(Vector3 outputValue)
{
    if(!actionScene || !smgr || !actionScene->isRigMode || actionScene->rigMan->sceneMode != RIG_MODE_PREVIEW)
        return false;
    
    bool isJointSelected = actionScene->hasJointSelected();
    bool isNodeSelected = actionScene->hasNodeSelected();
    SGNode* selectedNode = actionScene->getSelectedNode();
    SGJoint* selectedJoint = actionScene->getSelectedJoint();
    int selectedJointId = actionScene->rigMan->selectedJointId;
    SGNode* sgrSGNode = actionScene->rigMan->getRiggedNode();
    
    outputValue *= RADTODEG;
    if(isJointSelected){
        selectedJoint->jointNode->setRotationInDegrees(outputValue);
        selectedJoint->jointNode->updateAbsoluteTransformationOfChildren();
        if(getMirrorState() == MIRROR_ON){
            int mirrorJointId = BoneLimitsHelper::getMirrorJointId(selectedJointId);
            if(mirrorJointId != -1){
                shared_ptr<JointNode> mirrorNode = (dynamic_pointer_cast<AnimatedMeshNode>(sgrSGNode->node))->getJointNode(mirrorJointId);
                mirrorNode->setRotationInDegrees(outputValue * Vector3(1.0,-1.0,-1.0));
                mirrorNode->updateAbsoluteTransformationOfChildren();
                //mirrorNode.reset();
            }
        }
        return true;
    }
    else if(selectedNode){
        selectedNode->node->setRotationInDegrees(outputValue);
        if(isNodeSelected)
            selectedNode->node->updateAbsoluteTransformation();
        else
            selectedNode->node->updateAbsoluteTransformationOfChildren();
        return true;
    }
    return false;
}
