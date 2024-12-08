/*
 * Copyright 2024 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package android.gui;

import android.gui.JankData;

/** @hide */
interface IJankListener {

  /**
   * Callback reporting jank data of the most recent frames.
   * @See {@link ISurfaceComposer#addJankListener(IBinder, IJankListener)}
   */
  void onJankData(in JankData[] data);
}
