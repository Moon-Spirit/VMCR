// ===========================================================================
// VMCRPluginActivity.java - 空 Activity, 仅供 FCL 识别
// FCL 通过 AndroidManifest meta-data 识别插件, 不需要真正的 UI
// ===========================================================================
package com.mio.plugin.renderer.vmcr;

import android.app.Activity;
import android.os.Bundle;

/**
 * 占位 Activity, FCL 仅通过 AndroidManifest 扫描来识别插件.
 * 真正的渲染逻辑在 libGL.so / libEGL.so 中, 不需要任何 Java 代码.
 */
public final class VMCRPluginActivity extends Activity {
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        finish();
    }
}
