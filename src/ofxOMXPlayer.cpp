#include "ofxOMXPlayer.h"

ofxOMXPlayer::ofxOMXPlayer()
{
    playerID = 0;
    listener = NULL;
    engineNeedsRestart = false;
    pendingLoopMessage = false;
    OMX_Init();
    av_register_all();
    avformat_network_init();
    omxCore.Initialize();
    ofAddListener(ofEvents().update, this, &ofxOMXPlayer::onUpdate);
    
    
}

bool ofxOMXPlayer::setup(ofxOMXPlayerSettings settings_)
{
    
    settings = settings_;
    if(settings.listener)
    {
        listener = settings.listener;  
    }
    if(isOpen())
    {
        engine.close();
    }
    bool result = engine.setup(settings);
    if(result)
    {
        engine.listener = this; 
        if(settings.enableFilters)
        {
            currentFilterName = GetImageFilterString(engine.m_config_video.filterType);
        }
    }
    return result;
}


void ofxOMXPlayer::start()
{
    if(!engine.isThreadRunning())
    {
        engine.startThread();
    }
}
void ofxOMXPlayer::loadMovie(string videoPath)
{
    settings.videoPath = videoPath;
    engineNeedsRestart = true;
}

void ofxOMXPlayer::reopen()
{
    engineNeedsRestart = true;
}

#pragma mark GETTERS

int ofxOMXPlayer::getWidth()
{
    return engine.videoWidth; 
}

int ofxOMXPlayer::getHeight()
{
    return engine.videoHeight; 
}

float ofxOMXPlayer::getFPS()
{
    return engine.videoFrameRate;
}

int ofxOMXPlayer::getTotalNumFrames()
{
    return engine.totalNumFrames;
}

float ofxOMXPlayer::getDurationInSeconds()
{
    return engine.duration;
}

ofTexture& ofxOMXPlayer::getTextureReference()
{
    return engine.fbo.getTextureReference();
}
ofFbo& ofxOMXPlayer::getFboReference()
{
    return engine.fbo;
}

GLuint ofxOMXPlayer::getTextureID()
{
    return getTextureReference().getTextureData().textureID;
}

unsigned char* ofxOMXPlayer::getPixels()
{
    return engine.pixels;
}

int ofxOMXPlayer::getClockSpeed()
{
    return engine.omxClock.OMXPlaySpeed();
}

bool ofxOMXPlayer::isOpen()
{
    return engine.isOpen;
}

bool ofxOMXPlayer::getIsOpen()
{
    return isOpen();
}

bool ofxOMXPlayer::isPlaying()
{
    bool result = false;
    if(isOpen() && !isPaused() && engine.isThreadRunning())
    {
        result = true;
    }
    return result;
}

float ofxOMXPlayer::getPlaybackSpeed()
{
    return engine.speeds[engine.currentSpeed];
}




float ofxOMXPlayer::getMediaTime()
{
    float t = (float)(engine.omxClock.OMXMediaTime()*1e-6);
    return t;
}

int ofxOMXPlayer::getCurrentFrame()
{
    int result =0;
    int fps = getFPS();
    if(fps)
    {            
        result = getMediaTime()*fps;
    }
    return result;
}

float ofxOMXPlayer::getVolume()
{
    return getVolumeNormalized();
}

float ofxOMXPlayer::getVolumeDB()
{
    return engine.m_Volume;
}

bool ofxOMXPlayer::isLoopingEnabled()
{
    return engine.m_loop; 
}

bool ofxOMXPlayer::isTextureEnabled()
{
    return settings.enableTexture;
}

bool ofxOMXPlayer::isFrameNew()
{
    return engine.hasNewFrame;
}

COMXStreamInfo&  ofxOMXPlayer::getVideoStreamInfo()
{
    return engine.m_config_video.hints;
}
COMXStreamInfo&  ofxOMXPlayer::getAudioStreamInfo()
{
    return engine.m_config_audio.hints;
}

string ofxOMXPlayer::getRandomVideo(string path)
{
    string videoPath = "";
    
    ofDirectory currentVideoDirectory(ofToDataPath(path, true));
    if (currentVideoDirectory.exists())
    {
        currentVideoDirectory.listDir();
        currentVideoDirectory.sort();
        vector<ofFile> files = currentVideoDirectory.getFiles();
        if (files.size()>0)
        {
            if (files.size()==1)
            {
                videoPath = files[0].path();
            }else
            {
                int randomIndex = ofRandom(files.size());
                videoPath = files[randomIndex].path(); 
            }
            
        }
        
    }else
    {
        ofLogError(__func__) << "NO FILES FOUND AT" << currentVideoDirectory.path();
    }
    
    if(videoPath.empty())
    {
        ofLogWarning(__func__) << "returning empty string";
    }
    return videoPath;
}  

string ofxOMXPlayer::getInfo()
{
    stringstream info;
    info << "APP FPS: "+ ofToString(ofGetFrameRate()) << endl;
    if(isOpen())
    {
        int t = getMediaTime();
        info << "MEDIA TIME: " << (t/3600)<<"h:"<< (t/60)%60 <<"m:"<< t%60 <<":s"<<  " raw: " << getMediaTime() <<endl;
        
        info << "OMX CLOCK SPEED: " << getClockSpeed() << endl;
        info << "PLAYBACK SPEED: " << getPlaybackSpeed() << endl;
        
        
        info << "DIMENSIONS: " << getWidth()<<"x"<<getHeight();
        
        info << "FPS: " << getFPS() << endl;
        info << "DURATION IN SECS: " << getDurationInSeconds() << endl;
        info << "TOTAL FRAMES: " << getTotalNumFrames() << endl;
        info << "CURRENT FRAME: " << getCurrentFrame() << endl;
        if (getTotalNumFrames() > 0) 
        {
            info << "REMAINING FRAMES: " << getTotalNumFrames() - getCurrentFrame() << endl;
        }else 
        {
            info << "REMAINING FRAMES: N/A, NO TOTAL FRAMES" << endl;
        }        
        info << "LOOPING ENABLED: " << isLoopingEnabled() << endl;
        info << "CURRENT VOLUME RAW: " << getVolumeDB() << endl;
        info << "CURRENT VOLUME NORMALIZED: " << getVolumeNormalized() << endl; 
        info << "FILE: " << settings.videoPath << endl; 
        info << "TEXTURE ENABLED: " << isTextureEnabled() << endl; 
        info << "FILTERS ENABLED: " << settings.enableFilters << endl; 
        
        info << "FILTER: " << currentFilterName << endl; 
        
        
    }else
    {
        info << "CLOSED" << endl;
        
    }
    
    
    return info.str();
}

#pragma mark LISTENERS

void ofxOMXPlayer::onVideoEnd()
{
    if(listener)
    {
        listener->onVideoEnd(this);
    }
}

void ofxOMXPlayer::onVideoLoop(bool needsRestart)
{
    ofLogNotice(__func__) << endl;
    pendingLoopMessage = true;
    engineNeedsRestart = needsRestart;
}

void ofxOMXPlayer::onUpdate(ofEventArgs& eventArgs)
{
    if(engineNeedsRestart)
    {
        engineNeedsRestart = false;
        if(isOpen())
        {
            engine.close();
        }
        if(pendingLoopMessage)
        {
            if(listener)
            {
                listener->onVideoLoop(this);
            }
            pendingLoopMessage = false;
        }
        setup(settings);
        
        
    }else
    {
        if(pendingLoopMessage)
        {
            if(listener)
            {
                listener->onVideoLoop(this);
            }
            pendingLoopMessage = false;
        }
    }
    
}


void ofxOMXPlayer::close()
{
    engine.close();
}

void ofxOMXPlayer::enableLooping()
{
    engine.m_loop = true;
}

void ofxOMXPlayer::disableLooping()
{
    engine.m_loop = false; 
}

#pragma mark DRAWING

void ofxOMXPlayer::draw(float x, float y, float w, float h)
{
    //ofLog() << "draw: " << ofRectangle(x, y, w, h);
    if(isTextureEnabled())
    {
        engine.draw(x, y, w, h);
    }else
    {
        ofRectangle drawRect(x, y, w, h);
        draw(drawRect);
    }
}

void ofxOMXPlayer::draw(ofRectangle rectangle)
{
    if(isTextureEnabled())
    {
        draw(rectangle.x, rectangle.y, rectangle.width, rectangle.height);
        
    }else
    {
        ofRectangle cropRect;
        rectangle.width += rectangle.x;
        rectangle.height += rectangle.y;
        drawCropped(cropRect, rectangle);
        //ofLog() << rectangle;
    }
    
}

void ofxOMXPlayer::drawCropped(float cropX, float cropY, float cropWidth, float cropHeight,
                 float drawX, float drawY, float drawWidth, float drawHeight)
{
    engine.drawCropped(cropX, cropY, cropWidth, cropHeight,
                       drawX, drawY, drawWidth, drawHeight);
}

void ofxOMXPlayer::drawCropped(ofRectangle cropRectangle, ofRectangle drawRectangle)
{
    
    drawCropped(cropRectangle.x, cropRectangle.y, cropRectangle.width, cropRectangle.height,
                drawRectangle.x, drawRectangle.y, drawRectangle.width, drawRectangle.height);
}

void ofxOMXPlayer::setAlpha(int alpha)
{
    engine.setAlpha(alpha);
}

void ofxOMXPlayer::setLayer(int layer)
{
    if(!isOpen())
    {
        ofLogError() << "TOO EARLY TO SET LAYER, Use ofxOMXPlayerSettings.layer to pass in";
        return;
    }
    engine.setLayer(layer);
}

void ofxOMXPlayer::rotateVideo(int degrees, bool doMirror)
{
    if(isTextureEnabled())return;
    if(degrees<0) return;
    if(degrees>360) return;
    engine.rotateVideo(degrees, doMirror);
}

#pragma mark PLAYBACK CONTROLS

bool ofxOMXPlayer::isPaused()
{
    return engine.m_Pause;
}

void ofxOMXPlayer::setPaused(bool doPause)
{
    engine.m_Pause = doPause;
}

void ofxOMXPlayer::togglePause()
{
    engine.m_Pause = !engine.m_Pause;
}

void ofxOMXPlayer::setNormalSpeed()
{
    engine.setNormalSpeed();
}

void ofxOMXPlayer::increaseSpeed()
{
    engine.increaseSpeed();
}

void ofxOMXPlayer::decreaseSpeed()
{
    engine.decreaseSpeed();
}

void ofxOMXPlayer::stepFrameForward()
{
    stepNumFrames(1);
}

void ofxOMXPlayer::stepNumFrames(int step)
{
    engine.stepNumFrames(step);
}

void ofxOMXPlayer::seekToTimeInSeconds(int timeInSeconds)
{
    engine.seekToTimeInSeconds(timeInSeconds);
}

void ofxOMXPlayer::seekToFrame(int frameTarget)
{
    engine.seekToFrame(frameTarget);
}


void ofxOMXPlayer::restartMovie()
{
    if(getTotalNumFrames())
    {
       seekToFrame(0); 
    }else
    {
        reopen();
    }
    
}

#pragma mark PLAYBACK AUDIO

void ofxOMXPlayer::increaseVolume()
{
    engine.increaseVolume();
}

void ofxOMXPlayer::decreaseVolume()
{
    engine.decreaseVolume();
    
}

void ofxOMXPlayer::setVolumeNormalized(float volume)
{
    float value = ofMap(volume, 0.0, 1.0, -6000.0, 6000.0, true);
    engine.m_Volume = value;
    engine.applyVolume();
}

void ofxOMXPlayer::setVolume(float volume)
{
    setVolumeNormalized(volume);
}

float ofxOMXPlayer::getVolumeNormalized()
{
    float value = ofMap(engine.m_Volume, -6000.0, 6000.0, 0.0, 1.0, true);
    return value;
}



#pragma mark PIXELS

void ofxOMXPlayer::updatePixels()
{
    engine.updatePixels();
}


void ofxOMXPlayer::saveImage(string imagePath)
{
    if(!isTextureEnabled()) return;
    if(imagePath == "")
    {
        imagePath = ofToDataPath(ofGetTimestampString()+".png", true);
    }
    updatePixels();
    //TODO make smarter, re-allocating every time
    ofImage image;
    image.setFromPixels(getPixels(), getWidth(), getHeight(), OF_IMAGE_COLOR_ALPHA);
    image.saveImage(imagePath);
    
    ofLog() << "SAVED IMAGE TO: " << imagePath;
}


//void        draw(float x=0, float y=0);
//void        draw(ofRectangle&);

void ofxOMXPlayer::scrubForward(int step)
{
    ofLogError() << " scrubForward NOT IMPLEMENTED";
    
}
#pragma mark FILTERS
void ofxOMXPlayer::setFilter(OMX_IMAGEFILTERTYPE filterType)
{
    
    currentFilterName = GetImageFilterString(filterType);
    engine.setFilter(filterType);
}


void ofxOMXPlayer::setFilter(OMX_IMAGEFILTERTYPE filterType, vector<int> params)
{
    currentFilterName = GetImageFilterString(filterType);
    engine.setFilter(filterType, params);
}



COMXCoreComponent* ofxOMXPlayer::getVideoSplitter()
{
    COMXCoreComponent* videoSplitter=NULL;
    if(engine.m_player_video.m_decoder)
    {
        //ofLog() << "DECODER";
        videoSplitter = &engine.m_player_video.m_decoder->videoSplitter;
    }else
    {
        //ofLog() << "NO DECODER";

    }
    return videoSplitter;
}




