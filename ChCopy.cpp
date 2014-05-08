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

enum {
  kParameter_FromCh = 0,
  kParameter_ToCh = 1,
  kParameter_KeyRangeMin = 2,
  kParameter_KeyRangeMax = 3,
  kParameter_Transpose = 4,
  kParameter_Damper = 5,
  kParameter_OtherCC = 6,
  kParameter_PitchBend = 7,
  kParameter_OtherMessages = 8,
  kParameter_OriginChMute = 9,
  kNumberOfParameters = 10
};
// static const map<int, CFStringRef> kMapParam2Name;

static const CFStringRef kParamName_FromCh = CFSTR("Copy Channel From: ");
static const CFStringRef kParamName_ToCh = CFSTR("Copy Channel To: ");
static const CFStringRef kParamName_KeyRangeMin = CFSTR("Key Range Min: ");
static const CFStringRef kParamName_KeyRangeMax = CFSTR("Key Range Max: ");
static const CFStringRef kParamName_Transpose = CFSTR("Transpose: ");
static const CFStringRef kParamName_Damper = CFSTR("Damper");
static const CFStringRef kParamName_OtherCC = CFSTR("Other Control Changes");
static const CFStringRef kParamName_PitchBend = CFSTR("PitchBend");
static const CFStringRef kParamName_OtherMessages = CFSTR("Other Messages");
static const CFStringRef kParamName_OriginChMute = CFSTR("Origin Ch Mute");

ChCopy::ChCopy(AudioComponentInstance inComponentInstance)
    : AUMonotimbralInstrumentBase(inComponentInstance, 0,
                                  1) {  // should be 1 for output midi
  CreateElements();

  Globals()->UseIndexedParameters(kNumberOfParameters);

  Globals()->SetParameter(kParameter_FromCh, 1);
  Globals()->SetParameter(kParameter_ToCh, 2);
  Globals()->SetParameter(kParameter_KeyRangeMin, 1);
  Globals()->SetParameter(kParameter_KeyRangeMax, 127);
  Globals()->SetParameter(kParameter_Transpose, 0);
  Globals()->SetParameter(kParameter_Damper, 1);
  Globals()->SetParameter(kParameter_OtherCC, 1);
  Globals()->SetParameter(kParameter_PitchBend, 1);
  Globals()->SetParameter(kParameter_OtherMessages, 1);
  Globals()->SetParameter(kParameter_OriginChMute, 0);

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

#ifdef DEBUG
  DEBUGLOG_B("GetParameterInfo - inScope: " << inScope << endl);
  DEBUGLOG_B("GetParameterInfo - inParameterID: " << inParameterID << endl);
#endif

  if (inScope != kAudioUnitScope_Global) return kAudioUnitErr_InvalidScope;

  outParameterInfo.flags += kAudioUnitParameterFlag_IsWritable;
  outParameterInfo.flags += kAudioUnitParameterFlag_IsReadable;

  switch (inParameterID) {
    case kParameter_FromCh:
      AUBase::FillInParameterName(outParameterInfo, kParamName_FromCh, false);
      outParameterInfo.unit = kAudioUnitParameterUnit_Indexed;
      outParameterInfo.minValue = 1;
      outParameterInfo.maxValue = 16;
      break;
    case kParameter_ToCh:
      AUBase::FillInParameterName(outParameterInfo, kParamName_ToCh, false);
      outParameterInfo.unit = kAudioUnitParameterUnit_Indexed;
      outParameterInfo.minValue = 1;
      outParameterInfo.maxValue = 16;
      break;
    case kParameter_KeyRangeMin:
      AUBase::FillInParameterName(outParameterInfo, kParamName_KeyRangeMin,
                                  false);
      outParameterInfo.unit = kAudioUnitParameterUnit_MIDINoteNumber;
      outParameterInfo.minValue = 1;
      outParameterInfo.maxValue = 127;
      break;
    case kParameter_KeyRangeMax:
      AUBase::FillInParameterName(outParameterInfo, kParamName_KeyRangeMax,
                                  false);
      outParameterInfo.unit = kAudioUnitParameterUnit_MIDINoteNumber;
      outParameterInfo.minValue = 1;
      outParameterInfo.maxValue = 127;
      break;
    case kParameter_Transpose:
      AUBase::FillInParameterName(outParameterInfo, kParamName_Transpose,
                                  false);
      outParameterInfo.unit = kAudioUnitParameterUnit_MIDINoteNumber;
      outParameterInfo.minValue = -127;
      outParameterInfo.maxValue = 127;
      break;
    case kParameter_Damper:
      AUBase::FillInParameterName(outParameterInfo, kParamName_Damper, false);
      break;
    case kParameter_OtherCC:
      AUBase::FillInParameterName(outParameterInfo, kParamName_OtherCC, false);
      break;
    case kParameter_PitchBend:
      AUBase::FillInParameterName(outParameterInfo, kParamName_PitchBend,
                                  false);
      break;
    case kParameter_OtherMessages:
      AUBase::FillInParameterName(outParameterInfo, kParamName_OtherMessages,
                                  false);
      break;
    case kParameter_OriginChMute:
      AUBase::FillInParameterName(outParameterInfo, kParamName_OriginChMute,
                                  false);
      break;
    default:
      return kAudioUnitErr_InvalidParameter;
  }

  if (inParameterID == kParameter_Damper ||
      inParameterID == kParameter_OtherCC ||
      inParameterID == kParameter_PitchBend ||
      inParameterID == kParameter_OtherMessages ||
      inParameterID == kParameter_OriginChMute) {
    outParameterInfo.flags += SetAudioUnitParameterDisplayType(
        0, kAudioUnitParameterFlag_DisplaySquareRoot);
    outParameterInfo.unit = kAudioUnitParameterUnit_Boolean;
    outParameterInfo.minValue = 0;
    outParameterInfo.maxValue = 1;
  }

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

#ifdef DEBUG
  DEBUGLOG_B("HandleMidiEvent - status:"
             << (int)status << " ch:" << (int)channel << " data1:" << (int)data1
             << " data2:" << (int)data2 << endl);
#endif

  bool add = false;
  bool originChMute = false;
  int diff_data1 = 0;
  if (Globals()->GetParameter(kParameter_FromCh) - 1 == channel) {
    switch (status) {
      case 0x80:
      case 0x90: {
        int min = Globals()->GetParameter(kParameter_KeyRangeMin);
        int max = Globals()->GetParameter(kParameter_KeyRangeMax);
        if (data1 >= min && data1 <= max) {
          add = true;
          originChMute = true;
          diff_data1 = Globals()->GetParameter(kParameter_Transpose);
        }

#ifdef DEBUG
        DEBUGLOG_B("HandleMidiEvent - min:" << min << " max:" << max
                                            << " add:" << add << endl);
#endif
      } break;

      case 0xb0:
        if (Globals()->GetParameter(kParameter_Damper) && data1 == 64)
          add = true;
        else if (Globals()->GetParameter(kParameter_OtherCC) && data1 != 64)
          add = true;
        break;

      case 0xe0:
        if (Globals()->GetParameter(kParameter_PitchBend)) add = true;
        break;

      default:
        if (Globals()->GetParameter(kParameter_OtherMessages)) add = true;
    }
  }

  if (!Globals()->GetParameter(kParameter_OriginChMute) || !originChMute)
    mCallbackHelper.AddMIDIEvent(status, channel, data1, data2, inStartFrame);

  if (add)
    mCallbackHelper.AddMIDIEvent(status,
                                 Globals()->GetParameter(kParameter_ToCh) - 1,
                                 data1 + diff_data1, data2, inStartFrame);

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
