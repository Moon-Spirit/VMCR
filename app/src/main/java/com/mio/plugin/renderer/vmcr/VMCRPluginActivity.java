// ===========================================================================
// VMCRPluginActivity.java - 绌?Activity, 浠呬緵 FCL 璇嗗埆
// FCL 閫氳繃 AndroidManifest meta-data 璇嗗埆鎻掍欢, 涓嶉渶瑕佺湡姝ｇ殑 UI
// ===========================================================================
package com.mio.plugin.renderer.vmcr;

import android.app.Activity;
import android.os.Bundle;

/**
 * 鍗犱綅 Activity, FCL 浠呴€氳繃 AndroidManifest 鎵弿鏉ヨ瘑鍒彃浠?
 * 鐪熸鐨勬覆鏌撻€昏緫鍦?libGL.so / libEGL.so 涓? 涓嶉渶瑕佷换浣?Java 浠ｇ爜.
 */
public final class VMCRPluginActivity extends Activity {
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        // NoDisplay theme - 绔嬪嵆 finish
        finish();
    }
}

