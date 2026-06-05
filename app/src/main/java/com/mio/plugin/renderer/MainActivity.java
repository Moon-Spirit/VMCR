// ===========================================================================
// MainActivity.java - FCL 渲染器插件占位 Activity
//
// 严格按 https://github.com/ShirosakiMio/FCLRendererPlugin 模板.
// FCL 通过扫描 LAUNCHER intent-filter 发现 com.mio.plugin.renderer.* 包
// 命名的应用, 读 meta-data (renderer=...) 决定加载 libGL.so / libEGL.so.
// ===========================================================================
package com.mio.plugin.renderer;

import android.app.Activity;
import android.os.Bundle;

public final class MainActivity extends Activity {
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        finish();
    }
}
