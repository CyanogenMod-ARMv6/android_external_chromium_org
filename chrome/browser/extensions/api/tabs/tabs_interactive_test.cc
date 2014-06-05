// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/values.h"
#include "chrome/browser/extensions/api/tabs/tabs_api.h"
#include "chrome/browser/extensions/api/tabs/tabs_constants.h"
#include "chrome/browser/extensions/extension_apitest.h"
#include "chrome/browser/extensions/extension_function_test_utils.h"
#include "chrome/browser/extensions/extension_tab_util.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/interactive_test_utils.h"
#include "chrome/test/base/ui_test_utils.h"

namespace keys = extensions::tabs_constants;
namespace utils = extension_function_test_utils;

typedef InProcessBrowserTest ExtensionTabsTest;

// http://crbug.com/154081 for Aura specific
// http://crbug.com/179063 for other general failures on try bots.
#if defined(OS_WIN)
#define MAYBE_GetLastFocusedWindow DISABLED_GetLastFocusedWindow
#else
#define MAYBE_GetLastFocusedWindow GetLastFocusedWindow
#endif

IN_PROC_BROWSER_TEST_F(ExtensionTabsTest, MAYBE_GetLastFocusedWindow) {
  // Create a new window which making it the "last focused" window.
  // Note that "last focused" means the "top" most window.
  Browser* new_browser = CreateBrowser(browser()->profile());
  int focused_window_id =
      extensions::ExtensionTabUtil::GetWindowId(new_browser);

  scoped_refptr<extensions::WindowsGetLastFocusedFunction> function =
      new extensions::WindowsGetLastFocusedFunction();
  scoped_refptr<extensions::Extension> extension(utils::CreateEmptyExtension());
  function->set_extension(extension.get());
  scoped_ptr<base::DictionaryValue> result(utils::ToDictionary(
      utils::RunFunctionAndReturnSingleResult(function.get(),
                                              "[]",
                                              new_browser)));

  // The id should always match the last focused window and does not depend
  // on what was passed to RunFunctionAndReturnSingleResult.
  EXPECT_EQ(focused_window_id, utils::GetInteger(result.get(), "id"));
  base::ListValue* tabs = NULL;
  EXPECT_FALSE(result.get()->GetList(keys::kTabsKey, &tabs));

  function = new extensions::WindowsGetLastFocusedFunction();
  function->set_extension(extension.get());
  result.reset(utils::ToDictionary(
      utils::RunFunctionAndReturnSingleResult(function.get(),
                                              "[{\"populate\": true}]",
                                              browser())));

  // The id should always match the last focused window and does not depend
  // on what was passed to RunFunctionAndReturnSingleResult.
  EXPECT_EQ(focused_window_id, utils::GetInteger(result.get(), "id"));
  // "populate" was enabled so tabs should be populated.
  EXPECT_TRUE(result.get()->GetList(keys::kTabsKey, &tabs));
}

// Flaky: http://crbug.com/136562
IN_PROC_BROWSER_TEST_F(ExtensionTabsTest, DISABLED_QueryLastFocusedWindowTabs) {
  const size_t kExtraWindows = 2;
  for (size_t i = 0; i < kExtraWindows; ++i)
    CreateBrowser(browser()->profile());

  Browser* focused_window = CreateBrowser(browser()->profile());
#if defined(OS_MACOSX)
  // See BrowserWindowCocoa::Show. In tests, Browser::window()->IsActive won't
  // work unless we fake the browser being launched by the user.
  ASSERT_TRUE(ui_test_utils::ShowAndFocusNativeWindow(
      focused_window->window()->GetNativeWindow()));
#endif

  // Needed on Mac and Linux so that the BrowserWindow::IsActive calls work.
  content::RunAllPendingInMessageLoop();

  GURL url;
  AddTabAtIndexToBrowser(focused_window, 0, url, content::PAGE_TRANSITION_LINK);
  int focused_window_id =
      extensions::ExtensionTabUtil::GetWindowId(focused_window);

  // Get tabs in the 'last focused' window called from non-focused browser.
  scoped_refptr<extensions::TabsQueryFunction> function =
      new extensions::TabsQueryFunction();
  scoped_ptr<base::ListValue> result(utils::ToList(
      utils::RunFunctionAndReturnSingleResult(function.get(),
                                              "[{\"lastFocusedWindow\":true}]",
                                              browser())));

  base::ListValue* result_tabs = result.get();
  // We should have one initial tab and one added tab.
  EXPECT_EQ(2u, result_tabs->GetSize());
  for (size_t i = 0; i < result_tabs->GetSize(); ++i) {
    base::DictionaryValue* result_tab = NULL;
    EXPECT_TRUE(result_tabs->GetDictionary(i, &result_tab));
    EXPECT_EQ(focused_window_id, utils::GetInteger(result_tab,
                                                   keys::kWindowIdKey));
  }

  // Get tabs NOT in the 'last focused' window called from the focused browser.
  function = new extensions::TabsQueryFunction();
  result.reset(utils::ToList(
      utils::RunFunctionAndReturnSingleResult(function.get(),
                                              "[{\"lastFocusedWindow\":false}]",
                                              browser())));

  result_tabs = result.get();
  // We should get one tab for each extra window and one for the initial window.
  EXPECT_EQ(kExtraWindows + 1, result_tabs->GetSize());
  for (size_t i = 0; i < result_tabs->GetSize(); ++i) {
    base::DictionaryValue* result_tab = NULL;
    EXPECT_TRUE(result_tabs->GetDictionary(i, &result_tab));
    EXPECT_NE(focused_window_id, utils::GetInteger(result_tab,
                                                   keys::kWindowIdKey));
  }
}

#if defined(OS_WIN)  // http://crbug.com/154081 && http://crbug.com/171080
#define MAYBE_TabCurrentWindow DISABLED_TabCurrentWindow
#else
#define MAYBE_TabCurrentWindow TabCurrentWindow
#endif
IN_PROC_BROWSER_TEST_F(ExtensionApiTest, MAYBE_TabCurrentWindow) {
  ASSERT_TRUE(RunExtensionTest("tabs/current_window")) << message_;
}
