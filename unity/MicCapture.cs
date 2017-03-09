/* MicCapture.cs
 * Copyright Eddie Cameron 2014
 * ----------------------------
 *
 */

using UnityEngine;
using System;
using System.Collections;
using System.Collections.Generic;
using System.Runtime.InteropServices;


public class MicCapture : BehaviourBase
{
    private bool mEnabled;
    private int sampleRate;
    private static AsyncOperation getAuthOp;
#if !(UNITY_IPHONE && !UNITY_EDITOR)
    private const int MIC_REFRESH_TIME_S = 20;  
	public bool muteMic = true;
	private float[] mChanDemultiplexBuffer;
    public AudioClip testClip; // if no mic, use this
    private string micName = "";

    private AudioClip mMicRecClip;
    private int mMicRecClipIdx;
    private int mPlaybackClipIdx;

    private const int MIN_SAMPLES_TO_SEND = 1024;

    private void ASSERT(bool condition, string debugStr)
    {
        if (!condition)
        {
            DebugExtras.LogWarning("Assertion failed:" + debugStr);
        }
    }

#else
	[DllImport( "__Internal")]
	static extern int _IPhoneMicRead( System.IntPtr floatArrPtr, int length );
	[DllImport( "__Internal")]
	static extern int _IPhoneMicGetReadReady();
	[DllImport( "__Internal")]
	static extern int _IPhoneMicStart();
	[DllImport( "__Internal")]
	static extern void _IPhoneMicStop();

    private const int MIN_SAMPLES_TO_SEND = 1024;
    private float[] mSamplesReadBuffer = new float[MIN_SAMPLES_TO_SEND];    
#endif
    protected override void OnEnable() {
        base.OnEnable();

        if (!mEnabled) {
#if !(UNITY_IPHONE && !UNITY_EDITOR)
            mChanDemultiplexBuffer = new float[512];
            mMicRecClipIdx = mPlaybackClipIdx = 0;
#endif
            StartCoroutine(GetMicAccess());
        }

    }

    protected override void OnDisable() {
        base.OnDisable();
        if (mEnabled) {
            StopMic();
            mEnabled = false;
        }
    }

    protected override void Update() {
        base.Update();
#if !(UNITY_IPHONE && !UNITY_EDITOR)

        bool isPlaying = audio.isPlaying;
        bool isRecording = Microphone.IsRecording(micName);
        if (mEnabled) {
            if (!isRecording) {
               // Debug.Log("" + Time.time + ":******Waiting for microphone position********");
                mMicRecClip = Microphone.Start("", false, MIC_REFRESH_TIME_S, sampleRate);
                // Force audio to speaker
                iPhoneSpeaker.ForceToSpeaker();
                mMicRecClipIdx++;
                isRecording = true;
            }
            bool startPlaying = false;
            if (!isPlaying) {
                ASSERT(isRecording, " isRecording");
                if (Microphone.GetPosition(micName) >= MIN_SAMPLES_TO_SEND) {
                    startPlaying = true;
                }
            }
            else if (mMicRecClipIdx != mPlaybackClipIdx) {
                if (Microphone.GetPosition(micName) >= MIN_SAMPLES_TO_SEND) {
                    audio.Stop();
                    startPlaying = true;
                }
            }
            if (startPlaying) {
				//Debug.Log("" + Time.time + ":******Starting playback!********");
                audio.clip = mMicRecClip;
                mPlaybackClipIdx = mMicRecClipIdx;
                audio.Play();
            }
        } //if (mEnabled) 
        else {
            if ((isPlaying) || (isRecording)) {
                StopMic();
            }
          }
#else
      if (mEnabled) {
        if (sampleRate > 0) {
          if (_IPhoneMicGetReadReady() >= MIN_SAMPLES_TO_SEND) {
            GCHandle handle =
                GCHandle.Alloc(mSamplesReadBuffer, GCHandleType.Pinned);
            int samplesReturned = _IPhoneMicRead(handle.AddrOfPinnedObject(),
                                                 MIN_SAMPLES_TO_SEND);
            //HandleAudio(mChanDemultiplexBuffer, numFrames);
            handle.Free(); // TODO only do this once?
          }
        }
      } else {// if (mEnabled)
        if (sampleRate > 0) {
            Debug.Log("Stopping microphone.");
            StopMic();
        }
      }
#endif
    }


    #if !UNITY_WEBPLAYER || UNITY_EDITOR
    void OnAudioFilterRead(float[] samples, int channels) {
		#if!(UNITY_IPHONE && !UNITY_EDITOR)
        if (channels == 1) {
            // Mono data can be sent directly.
            //HandleAudio(samples, samples.Length);
        }
        else {
            // Multiple channels, we only want the first one!
            int numFrames = samples.Length / channels;
            ASSERT(numFrames * channels == samples.Length, "channel mismatch");
            if ((null == mChanDemultiplexBuffer) || (mChanDemultiplexBuffer.Length < numFrames)) {
                mChanDemultiplexBuffer = new float[2 * numFrames];
            }
            for (int i = 0; i < numFrames; i++) {
                // Demultiplex into the demultiplex buffer.
                mChanDemultiplexBuffer[i] = samples[i * channels];
            }
            //HandleAudio(mChanDemultiplexBuffer, numFrames);
        }

        // If muteMic is requested, then mute the mic!
        if (muteMic) {
            for (int i = 0; i < samples.Length; i++) {
                samples[i] = 0;
            }
        }
        #endif
    }
    #endif

    IEnumerator GetMicAccess() {
        DebugExtras.Log( "Getting access to mic" );
        #if UNITY_WEBPLAYER && !UNITY_EDITOR
        if ( !Application.HasUserAuthorization( UserAuthorization.Microphone ) ) {
            if ( getAuthOp == null )
                getAuthOp = Application.RequestUserAuthorization( UserAuthorization.Microphone );
            yield return getAuthOp;
            if ( Application.HasUserAuthorization( UserAuthorization.Microphone ) ) {
                DebugExtras.Log( "Mic access accepted" );
            }
            else {
                DebugExtras.LogWarning( "Mic access denied" );
                yield break;
            }
        }
        if ( !enabled )
            yield break;
        #endif

        // TODO show mic chooser

#if !(UNITY_IPHONE && !UNITY_EDITOR)
        sampleRate = AudioSettings.outputSampleRate;
        int minFreq, maxFreq;
        Microphone.GetDeviceCaps( "", out minFreq, out maxFreq );
        if (maxFreq > 0 && maxFreq < sampleRate) { 
            AudioSettings.outputSampleRate = maxFreq; // take audio sample rate down to meet mic's
        }
#else
        sampleRate = _IPhoneMicStart();
        Debug.Log( "_IPhoneMicStart() returned FS of " + sampleRate + "Hz." );
#endif

        // We set enabled to be TRUE and we simply wait until Update() starts the microphone.
        mEnabled = true;
        yield return 0;
    }


    void StopMic() {
#if !(UNITY_IPHONE && !UNITY_EDITOR)
        audio.Stop();
        Microphone.End("");
#else
        if(sampleRate > 0) {
            sampleRate = 0;
            _IPhoneMicStop();
        }
#endif
        mEnabled = false;
    }
}
