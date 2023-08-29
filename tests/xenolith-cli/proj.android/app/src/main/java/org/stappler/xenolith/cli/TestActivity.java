package org.stappler.xenolith.cli;

import android.os.Bundle;

public class TestActivity extends android.app.Activity {
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
	}

	@Override
	protected void onStart() {
		super.onStart();

		run();
	}

	@Override
	protected void onResume() {
		super.onResume();
	}

	@Override
	protected void onDestroy() {
		super.onDestroy();
	}

	protected native void run();

	static {
		System.loadLibrary("application");
	}
}
