package micwrapper;

/**
 * Created by chris on 18/06/15.
 */
public class MicW {

    // Starts the microphone with sample rate fs, returns the actual sample rate.
    public static native int Start( int fs );
    public static native int Stop();
    public static native int GetReadReady();
    public static native int Read(float fbuf[], int length);

}
