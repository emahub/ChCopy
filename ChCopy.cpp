#include "AUInstrumentBase.h"
#include "ChCopyVersion.h"
#include "MIDIOutputCallbackHelper.h"
#include <CoreMIDI/CoreMIDI.h>
#include <vector>

#ifdef DEBUG
#include <fstream>
#include <ctime>
#endif

#ifdef DEBUG
#define DEBUGLOG_B(x) \
  if (baseDebugFile.is_open()) baseDebugFile << x
#else
#define DEBUGLOG_B(x)
#endif

using namespace std;

class ChCopy : public AUMonotimbralInstrumentBase {
 public:
  ChCopy(AudioUnit inComponentInstance);
  ~ChCopy();

  OSStatus GetPropertyInfo(AudioUnitPropertyID inID, AudioUnitScope inScope,
                           AudioUnitElement inElement, UInt32 &outDataSize,
                           Boolean &outWritable);

  OSStatus GetProperty(AudioUnitPropertyID inID, AudioUnitScope inScope,
                       AudioUnitElement inElement, void *outData);

  OSStatus SetProperty(AudioUnitPropertyID inID, AudioUnitScope inScope,
                       AudioUnitElement inElement, const void *inData,
                       UInt32 inDataSize);

  OSStatus HandleMidiEvent(UInt8 status, UInt8 channel, UInt8 data1,
                           UInt8 data2, UInt32 inStartFrame);

  OSStatus Render(AudioUnitRenderActionFlags &ioActionFlags,
                  const AudioTimeStamp &inTimeStamp, UInt32 inNumberFrames);

  OSStatus Initialize();
  void Cleanup();
  OSStatus Version() { return kChCopyVersion; }

  AUElement *CreateElement(AudioUnitScope scope, AudioUnitElement element);

  OSStatus GetParameterInfo(AudioUnitScope inScope,
                            AudioUnitParameterID inParameterID,
                            AudioUnitParameterInfo &outParameterInfo);

  MidiControls *GetControls(MusicDeviceGroupID inChannel) {
    SynthGroupElement *group = GetElForGroupID(inChannel);
    return (MidiControls *)group->GetMIDIControlHandler();
  }

 private:
  MIDIOutputCallbackHelper mCallbackHelper;

 protected:
#ifdef DEBUG
  ofstream baseDebugFile;
#endif
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#pragma mark ChCopy Methods
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

AUDIOCOMPONENT_ENTRY(AUMusicDeviceFactory, ChCopy)

ChCopy::ChCopy(AudioComponentInstance inComponentInstance)
    : AUMonotimbralInstrumentBase(inComponentInstance, 0,
                                  1) {  // should be 1 for output midi
  CreateElements();

  Globals()->UseIndexedParameters(16);  // we're defining 16 param

#ifdef DEBUG
  string bPath, bFullFileName;
  bPath = getenv("HOME");
  if (!bPath.empty()) {
    bFullFileName = bPath + "/Desktop/" + "Debug.log";
  } else {
    bFullFileName = "Debug.log";
  }

  baseDebugFile.open(bFullFileName.c_str());
  DEBUGLOG_B("Plug-in constructor invoked with parameters:" << endl);
#endif
}

ChCopy::~ChCopy() {
#ifdef DEBUG
  DEBUGLOG_B("ChCopy::~ChCopy" << endl);
#endif
}

OSStatus ChCopy::GetPropertyInfo(AudioUnitPropertyID inID,
                                 AudioUnitScope inScope,
                                 AudioUnitElement inElement,
                                 UInt32 &outDataSize, Boolean &outWritable) {
  if (inScope == kAudioUnitScope_Global) {
    if (inID == kAudioUnitProperty_MIDIOutputCallbackInfo) {
      outDataSize = sizeof(CFArrayRef);
      outWritable = false;
      return noErr;
    } else if (inID == kAudioUnitProperty_MIDIOutputCallback) {
      outDataSize = sizeof(AUMIDIOutputCallbackStruct);
      outWritable = true;
      return noErr;
    }
  }
  return AUMonotimbralInstrumentBase::GetPropertyInfo(inID, inScope, inElement,
                                                      outDataSize, outWritable);
}

void ChCopy::Cleanup() {
#ifdef DEBUG
  DEBUGLOG_B("ChCopy::Cleanup" << endl);
#endif
}

OSStatus ChCopy::Initialize() {
#ifdef DEBUG
  DEBUGLOG_B("->ChCopy::Initialize" << endl);
#endif

  AUMonotimbralInstrumentBase::Initialize();

#ifdef DEBUG
  DEBUGLOG_B("<-ChCopy::Initialize" << endl);
#endif

  return noErr;
}

AUElement *ChCopy::CreateElement(AudioUnitScope scope,
                                 AudioUnitElement element) {
#ifdef DEBUG
  DEBUGLOG_B("CreateElement - scope: " << scope << endl);
#endif
  switch (scope) {
    case kAudioUnitScope_Group:
      return new SynthGroupElement(this, element, new MidiControls);
    case kAudioUnitScope_Part:
      return new SynthPartElement(this, element);
    default:
      return AUBase::CreateElement(scope, element);
  }
}

OSStatus ChCopy::GetParameterInfo(AudioUnitScope inScope,
                                  AudioUnitParameterID inParameterID,
                                  AudioUnitParameterInfo &outParameterInfo) {

  if (inParameterID >= 16) return kAudioUnitErr_InvalidParameter;

  if (inScope != kAudioUnitScope_Global) return kAudioUnitErr_InvalidScope;

  outParameterInfo.flags = SetAudioUnitParameterDisplayType(
      0, kAudioUnitParameterFlag_DisplaySquareRoot);
  outParameterInfo.flags += kAudioUnitParameterFlag_IsWritable;
  outParameterInfo.flags += kAudioUnitParameterFlag_IsReadable;

  CFStringRef cfs =
      CFStringCreateWithFormat(NULL, NULL, CFSTR("Ch%d"), inParameterID + 1);
  AUBase::FillInParameterName(outParameterInfo, cfs, false);
  outParameterInfo.unit = kAudioUnitParameterUnit_Boolean;
  outParameterInfo.minValue = 0;
  outParameterInfo.maxValue = 1;
  // default value should be set in construtor with SetParameter method.
  // outParameterInfo.defaultValue = 1;

  return noErr;
}

OSStatus ChCopy::GetProperty(AudioUnitPropertyID inID, AudioUnitScope inScope,
                             AudioUnitElement inElement, void *outData) {
  if (inScope == kAudioUnitScope_Global) {
    if (inID == kAudioUnitProperty_MIDIOutputCallbackInfo) {
      CFStringRef strs[1];
      strs[0] = CFSTR("MIDI Callback");

      CFArrayRef callbackArray =
          CFArrayCreate(NULL, (const void **)strs, 1, &kCFTypeArrayCallBacks);
      *(CFArrayRef *)outData = callbackArray;
      return noErr;
    }
  }
  return AUMonotimbralInstrumentBase::GetProperty(inID, inScope, inElement,
                                                  outData);
}

OSStatus ChCopy::SetProperty(AudioUnitPropertyID inID, AudioUnitScope inScope,
                             AudioUnitElement inElement, const void *inData,
                             UInt32 inDataSize) {
#ifdef DEBUG
  DEBUGLOG_B("SetProperty" << endl);
#endif
  if (inScope == kAudioUnitScope_Global) {
    if (inID == kAudioUnitProperty_MIDIOutputCallback) {
      if (inDataSize < sizeof(AUMIDIOutputCallbackStruct))
        return kAudioUnitErr_InvalidPropertyValue;

      AUMIDIOutputCallbackStruct *callbackStruct =
          (AUMIDIOutputCallbackStruct *)inData;
      mCallbackHelper.SetCallbackInfo(callbackStruct->midiOutputCallback,
                                      callbackStruct->userData);
      return noErr;
    }
  }
  return AUMonotimbralInstrumentBase::SetProperty(inID, inScope, inElement,
                                                  inData, inDataSize);
}

OSStatus ChCopy::HandleMidiEvent(UInt8 status, UInt8 channel, UInt8 data1,
                                 UInt8 data2, UInt32 inStartFrame) {
  // data1 : note number, data2 : velocity

  // snag the midi event and then store it in a vector
  if (!Globals()->GetParameter(channel) ||
      ((status != 0x80 && status != 0x90) || (status == 0x80 && data1 != 0x00)))
    mCallbackHelper.AddMIDIEvent(status, channel, data1, data2, inStartFrame);

  return AUMIDIBase::HandleMidiEvent(status, channel, data1, data2,
                                     inStartFrame);
}

OSStatus ChCopy::Render(AudioUnitRenderActionFlags &ioActionFlags,
                        const AudioTimeStamp &inTimeStamp,
                        UInt32 inNumberFrames) {

  OSStatus result =
      AUInstrumentBase::Render(ioActionFlags, inTimeStamp, inNumberFrames);
  if (result == noErr) {
    mCallbackHelper.FireAtTimeStamp(inTimeStamp);
  }
  return result;
}
