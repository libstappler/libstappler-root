/**
 Copyright (c) 2023 Stappler LLC <admin@stappler.dev>

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 **/

#if ANDROID
#include <jni.h>
#include <android/native_activity.h>

#include "android/XLPlatformAndroidActivity.h"
#include "android/XLPlatformAndroidMessageInterface.h"
#include "ExampleApplication.h"

namespace stappler::xenolith::app {

SP_EXTERN_C JNIEXPORT
void ANativeActivity_onCreate(ANativeActivity* activity, void* savedState, size_t savedStateSize) {
	stappler::log::format(log::Debug, "NativeActivity", "Creating: %p %s %s", activity, activity->internalDataPath, activity->externalDataPath);

	auto a = Rc<platform::Activity>::create(activity, platform::ActivityFlags::CaptureInput);
	auto info = a->getActivityInfo();

	a->run([info = move(info)] (platform::Activity *a, Function<void()> &&initCb) {
		ViewCommandLineData appInfo({
			.bundleName = move(info.bundleName),
			.applicationName = move(info.applicationName),
			.applicationVersion = move(info.applicationVersion),
			.userLanguage = move(info.locale),
			.userAgent = move(info.systemAgent),
			.density = info.density
		});

		auto app = Rc<ExampleApplication>::create(move(appInfo), a);
		app->run(move(initCb));
	});
}

}

#endif
