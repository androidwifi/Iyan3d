//
//  SGSceneLoader.cpp
//  Iyan3D
//
//  Created by Karthik on 26/12/15.
//  Copyright © 2015 Smackall Games. All rights reserved.
//

#include "SGSceneLoader.h"
#include "SGEditorScene.h"
#include "SGAnimationSceneHelper.h"

SGEditorScene *currentScene;

SGSceneLoader::SGSceneLoader(SceneManager* sceneMngr, void* scene)
{
    this->smgr = sceneMngr;
    currentScene = (SGEditorScene*)scene;
}

SGSceneLoader::~SGSceneLoader()
{
    
}

bool SGSceneLoader::loadSceneData(std::string *filePath)
{
    if(!currentScene || !smgr)
        return;

    ifstream inputSGBFile(*filePath,ios::in | ios::binary );
    FileHelper::resetSeekPosition();
    if(!readScene(&inputSGBFile)){
        inputSGBFile.close();
        return false;
    }
    inputSGBFile.close();
    return true;
}

bool SGSceneLoader::readScene(ifstream *filePointer)
{
    if(!currentScene || !smgr)
        return false;

    int nodeCount = 0;
    readSceneGlobalInfo(filePointer, nodeCount);
    currentScene->totalFrames = (currentScene->totalFrames < 24) ? 24 : currentScene->totalFrames;
    currentScene->nodes.clear();
    if(nodeCount < NODE_LIGHT+1)
        return false;
    
    vector<SGNode*> tempNodes;
    for(int i = 0;i < nodeCount;i++){
        SGNode *sgNode = new SGNode(NODE_UNDEFINED);
        sgNode->readData(filePointer);
        bool status = true;
        
        if(sgNode->getType() == NODE_SGM || sgNode->getType() == NODE_RIG || sgNode->getType() == NODE_OBJ)
        {
            status = currentScene->downloadMissingAssetCallBack(to_string(sgNode->assetId),sgNode->getType());
        }
        else if (sgNode->getType() == NODE_TEXT) {
            status = currentScene->downloadMissingAssetCallBack(sgNode->optionalFilePath,sgNode->getType());
        } else if (sgNode->getType() == NODE_IMAGE) {
            status = currentScene->downloadMissingAssetCallBack(ConversionHelper::getStringForWString(sgNode->name), sgNode->getType());
        }
        
        if(!status)
            sgNode = NULL;
        tempNodes.push_back(sgNode);
    }
    for (int i = 0; i < tempNodes.size(); i++) {
        SGNode *sgNode = tempNodes[i];
        bool nodeLoaded = false;
        if(sgNode)
            nodeLoaded = loadNode(sgNode, OPEN_SAVED_FILE);
        
        if(!nodeLoaded)
            delete sgNode;
    }
    
    return true;
}

#ifdef ANDROID
bool SGSceneLoader::loadSceneData(std::string *filePath, JNIEnv *env, jclass type)
{
    ifstream inputSGBFile(*filePath,ios::in | ios::binary );
    FileHelper::resetSeekPosition();
    if(!readScene(&inputSGBFile, env, type)){
        inputSGBFile.close();
        return false;
    }
    inputSGBFile.close();
    return true;
}
bool SGSceneLoader::readScene(ifstream *filePointer, JNIEnv *env, jclass type)
{
    if(!currentScene || !smgr)
        return false;
    
    int nodeCount = 0;
    readSceneGlobalInfo(filePointer, nodeCount);
    currentScene->totalFrames = (currentScene->totalFrames < 24) ? 24 : currentScene->totalFrames;
    currentScene->nodes.clear();
    if(nodeCount < NODE_LIGHT+1)
        return false;
    
    vector<SGNode*> tempNodes;
    for(int i = 0;i < nodeCount;i++){
        SGNode *sgNode = new SGNode(NODE_UNDEFINED);
        sgNode->readData(filePointer);
        bool status = true;
        
        if(sgNode->getType() == NODE_SGM || sgNode->getType() == NODE_RIG || sgNode->getType() == NODE_OBJ)
        {
            status = currentScene->downloadMissingAssetsCallBack(to_string(sgNode->assetId),sgNode->getType(), env, type);
        }
        else if (sgNode->getType() == NODE_TEXT) {
            
            status = currentScene->downloadMissingAssetsCallBack(sgNode->optionalFilePath,sgNode->getType(),env,type);
        } else if (sgNode->getType() == NODE_IMAGE) {
            status = currentScene->downloadMissingAssetsCallBack(ConversionHelper::getStringForWString(sgNode->name), sgNode->getType(), env, type);
        }
        
        if(!status)
            sgNode = NULL;
        tempNodes.push_back(sgNode);
    }
    for (int i = 0; i < tempNodes.size(); i++) {
        SGNode *sgNode = tempNodes[i];
        bool nodeLoaded = false;
        if(sgNode)
            nodeLoaded = loadNode(sgNode,OPEN_SAVED_FILE);
        
        if(!nodeLoaded)
            delete sgNode;
    }
    
    return true;
}
#endif

void SGSceneLoader::readSceneGlobalInfo(ifstream *filePointer, int& nodeCount)
{
    if(!currentScene || !smgr)
        return;

    currentScene->totalFrames = FileHelper::readInt(filePointer);
    Vector3 lightColor;
    lightColor.x = FileHelper::readFloat(filePointer);
    lightColor.y = FileHelper::readFloat(filePointer);
    lightColor.z = FileHelper::readFloat(filePointer);
    ShaderManager::lightColor.push_back(lightColor);
    ShaderManager::shadowDensity = FileHelper::readFloat(filePointer);
    currentScene->cameraFOV = FileHelper::readFloat(filePointer);
    nodeCount = FileHelper::readInt(filePointer);
    
}

SGNode* SGSceneLoader::loadNode(NODE_TYPE type,int assetId,wstring name,int imgwidth,int imgheight,int actionType, Vector4 textColor, string fontFilePath)
{
    if(!currentScene || !smgr)
        return;
    currentScene->freezeRendering = true;
    SGNode *sgnode = new SGNode(type);
    sgnode->node = sgnode->loadNode(assetId,type,smgr,name,imgwidth,imgheight,textColor,fontFilePath);
    if(!sgnode->node){
        delete sgnode;
        Logger::log(INFO,"SGANimationScene","Node not loaded");
        return NULL;
    }
    if(sgnode->getType() == NODE_TEXT)
        currentScene->textJointsBasePos[(int)currentScene->nodes.size()] = SGAnimationSceneHelper::storeTextInitialPositions(sgnode);
    sgnode->assetId = assetId;
    sgnode->name = name;
    sgnode->setInitialKeyValues(actionType);
    sgnode->node->updateAbsoluteTransformation();
    sgnode->node->updateAbsoluteTransformationOfChildren();
    if(type == NODE_CAMERA)
        ShaderManager::camPos = sgnode->node->getAbsolutePosition();
    else if (type == NODE_LIGHT){
        currentScene->initLightCamera(sgnode->node->getPosition());
        addLight(sgnode);
    }else if(type == NODE_IMAGE){
        sgnode->props.isLighting = false;
    } else if (type == NODE_RIG) {
        dynamic_pointer_cast<AnimatedMeshNode>(sgnode->node)->updateMeshCache(CHARACTER_RIG);
    } else if (type == NODE_TEXT) {
        dynamic_pointer_cast<AnimatedMeshNode>(sgnode->node)->updateMeshCache(TEXT_RIG);
    } else if (type == NODE_ADDITIONAL_LIGHT) {
        addLight(sgnode);
    }
    
    //if (type >= NODE_LIGHT && type != NODE_ADDITIONAL_LIGHT)
    sgnode->node->setTexture(currentScene->shadowTexture,2);
    
    if(actionType != UNDO_ACTION && actionType != REDO_ACTION)
        sgnode->actionId = ++currentScene->actionObjectsSize;
    currentScene->nodes.push_back(sgnode);
    
    sgnode->node->setID(currentScene->assetIDCounter++);
    performUndoRedoOnNodeLoad(sgnode,actionType);
    currentScene->updater->setDataForFrame(currentScene->currentFrame);
    currentScene->updater->resetMaterialTypes(false);
    currentScene->freezeRendering = false;
    return sgnode;
}

bool SGSceneLoader::loadNode(SGNode *sgNode,int actionType)
{
    if(!currentScene || !smgr)
        return;

    Vector4 nodeSpecificColor = Vector4(sgNode->props.vertexColor.x,sgNode->props.vertexColor.y,sgNode->props.vertexColor.z,1.0);
    sgNode->node = sgNode->loadNode(sgNode->assetId,sgNode->getType(),smgr,sgNode->name,sgNode->props.fontSize,sgNode->props.nodeSpecificFloat,nodeSpecificColor,sgNode->optionalFilePath);
    
    if(!sgNode->node){
        Logger::log(INFO,"SGANimationScene","Node not loaded");
        return false;
    }
    if(sgNode->getType() == NODE_TEXT)
        currentScene->textJointsBasePos[(int)currentScene->nodes.size()] = SGAnimationSceneHelper::storeTextInitialPositions(sgNode);
    
    sgNode->setInitialKeyValues(actionType);
    sgNode->node->updateAbsoluteTransformation();
    sgNode->node->updateAbsoluteTransformationOfChildren();
    //if(sgNode->getType() >= NODE_LIGHT)
    sgNode->node->setTexture(currentScene->shadowTexture,2);
    sgNode->node->setVisible(true);
    if(actionType != UNDO_ACTION && actionType != REDO_ACTION)
        sgNode->actionId = ++currentScene->actionObjectsSize;
    currentScene->nodes.push_back(sgNode);
    sgNode->node->setID(currentScene->assetIDCounter++);
    performUndoRedoOnNodeLoad(sgNode,actionType);
    if(sgNode->getType() == NODE_LIGHT) {
        currentScene->initLightCamera(sgNode->node->getPosition());
        addLight(sgNode);
    } else if(sgNode->getType() == NODE_ADDITIONAL_LIGHT)
        addLight(sgNode);
    else if(sgNode->getType() == NODE_IMAGE && actionType != OPEN_SAVED_FILE && actionType != UNDO_ACTION)
        sgNode->props.isLighting = false;
    
    currentScene->updater->setDataForFrame(currentScene->currentFrame);
    currentScene->updater->resetMaterialTypes(false);
    
    return true;
}

void SGSceneLoader::addLight(SGNode *light)
{
    if(!currentScene || !smgr)
        return;
    
    Quaternion rotation = KeyHelper::getKeyInterpolationForFrame<int, SGRotationKey, Quaternion>(currentScene->currentFrame,light->rotationKeys,true);
    Vector3 scale = KeyHelper::getKeyInterpolationForFrame<int, SGScaleKey, Vector3>(currentScene->currentFrame, light->scaleKeys);
    Vector3 lightColor = (light->getType() == NODE_LIGHT) ? Vector3(scale.x,scale.y,scale.z) : Vector3(rotation.x, rotation.y, rotation.z);
    float fadeDistance = (light->getType() == NODE_LIGHT) ? 999.0 : rotation.w;
    
    ShaderManager::lightPosition.push_back(light->node->getAbsolutePosition());
    ShaderManager::lightColor.push_back(Vector3(lightColor.x,lightColor.y,lightColor.z));
    ShaderManager::lightFadeDistances.push_back(fadeDistance);
}

void SGSceneLoader::performUndoRedoOnNodeLoad(SGNode* meshObject,int actionType)
{
    if(!currentScene || !smgr)
        return;

    if(actionType == UNDO_ACTION) {
        int jointsCnt = (int)meshObject->joints.size();
        SGAction &deleteAction = currentScene->actions[currentScene->currentAction-1];
        if(deleteAction.nodePositionKeys.size())
            meshObject->positionKeys = deleteAction.nodePositionKeys;
        if(deleteAction.nodeRotationKeys.size())
            meshObject->rotationKeys = deleteAction.nodeRotationKeys;
        if(deleteAction.nodeSCaleKeys.size())
            meshObject->scaleKeys = deleteAction.nodeSCaleKeys;
        if(deleteAction.nodeVisibilityKeys.size())
            meshObject->visibilityKeys = deleteAction.nodeVisibilityKeys;
        for(int i = 0; i < jointsCnt; i++){
            if(deleteAction.jointRotKeys.find(i) != deleteAction.jointRotKeys.end() && deleteAction.jointRotKeys[i].size()) {
                //meshObject->joints[i]->rotationKeys = deleteAction.jointsRotationKeys[i];
                meshObject->joints[i]->rotationKeys = deleteAction.jointRotKeys[i];
            }
            if(meshObject->getType() == NODE_TEXT) {
                if(deleteAction.jointPosKeys.find(i) != deleteAction.jointPosKeys.end())
                    meshObject->joints[i]->positionKeys = deleteAction.jointPosKeys[i];
                if(deleteAction.jointScaleKeys.find(i) != deleteAction.jointScaleKeys.end())
                    meshObject->joints[i]->scaleKeys = deleteAction.jointScaleKeys[i];
            }
        }
        meshObject->props.prevMatName = ConversionHelper::getStringForWString(deleteAction.actionSpecificStrings[0]);
        meshObject->actionId = currentScene->actions[currentScene->currentAction-1].objectIndex;
        currentScene->currentAction--;
    }
    
    if(actionType == REDO_ACTION) {
        SGAction &deleteAction = currentScene->actions[currentScene->currentAction];
        meshObject->props.prevMatName = ConversionHelper::getStringForWString(deleteAction.actionSpecificStrings[0]);
        meshObject->actionId = deleteAction.objectIndex;
        currentScene->currentAction++;
    }
}

bool SGSceneLoader::removeObject(u16 nodeIndex, bool deAllocScene)
{
    if(!currentScene || !smgr || nodeIndex >= currentScene->nodes.size())
        return false;

    currentScene->renHelper->setControlsVisibility(false);
    currentScene->renHelper->setJointSpheresVisibility(false);
    
    SGNode * currentNode = currentScene->nodes[nodeIndex];
    if(currentNode->getType() == NODE_ADDITIONAL_LIGHT) {
        //popLightProps();
        currentScene->updater->resetMaterialTypes(false);
    }
    
    if(currentNode->getType() != NODE_TEXT && currentNode->getType() != NODE_ADDITIONAL_LIGHT)
        smgr->RemoveTexture(currentNode->node->getActiveTexture());
    
    smgr->RemoveNode(currentNode->node);
    delete currentNode;
    if(!deAllocScene) {
        currentScene->nodes.erase(currentScene->nodes.begin() + nodeIndex);
    }
    currentScene->updater->updateLightProperties(currentScene->currentFrame);
    currentScene->selectedNodeId = NOT_EXISTS;
    currentScene->selectedNode = NULL;
    currentScene->selectedJoint = NULL;
    currentScene->isNodeSelected = currentScene->isJointSelected = false;
    if(!deAllocScene)
        currentScene->updater->reloadKeyFrameMap();
    return true;
}


