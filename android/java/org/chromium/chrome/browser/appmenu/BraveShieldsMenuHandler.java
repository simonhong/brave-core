/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.appmenu;

import android.app.Activity;
import android.content.Context;
import android.view.Menu;
import android.view.View;
import android.widget.PopupMenu;
import android.view.ContextThemeWrapper;
import android.graphics.Point;
import android.graphics.Rect;
import android.widget.ListPopupWindow;
import android.widget.PopupWindow;
import android.graphics.drawable.Drawable;
import android.view.MenuItem;
import android.view.LayoutInflater;
import android.animation.Animator;
import android.animation.Animator.AnimatorListener;
import android.animation.AnimatorSet;
import android.os.Build;
import android.view.ViewGroup;
import android.view.Surface;
import android.widget.TextView;

import org.chromium.base.SysUtils;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.preferences.website.BraveShieldsContentSettings;
import org.chromium.chrome.R;
import org.chromium.base.AnimationFrameTimeHistogram;

import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * Object responsible for handling the creation, showing, hiding of the BraveShields menu.
 */
public class BraveShieldsMenuHandler {

    private static class BlockersInfo {
        public BlockersInfo() {
            mAdsBlocked = 0;
            mTrackersBlocked = 0;
            mHTTPSUpgrades = 0;
            mScriptsBlocked = 0;
            mFingerprintsBlocked = 0;
        }

        public int mAdsBlocked;
        public int mTrackersBlocked;
        public int mHTTPSUpgrades;
        public int mScriptsBlocked;
        public int mFingerprintsBlocked;
    }

    private final static float LAST_ITEM_SHOW_FRACTION = 0.5f;

    private final Context mContext;
    private final int mMenuResourceId;
    private Menu mMenu;
    private ListPopupWindow mPopup;
    private BraveShieldsMenuAdapter mAdapter;
    private AnimatorSet mMenuItemEnterAnimator;
    private AnimatorListener mAnimationHistogramRecorder = AnimationFrameTimeHistogram
            .getAnimatorRecorder("WrenchMenu.OpeningAnimationFrameTimes");
    private BraveShieldsMenuObserver mMenuObserver;
    private View mHardwareButtonMenuAnchor;
    private final Map<Integer, BlockersInfo> mTabsStat =
            Collections.synchronizedMap(new HashMap<Integer, BlockersInfo>());

    /**
     * Constructs a BraveShieldsMenuHandler object.
     * @param context Context that is using the BraveShieldsMenu.
     * @param menuResourceId Resource Id that should be used as the source for the menu items.
     */
    public BraveShieldsMenuHandler(Context context, int menuResourceId) {
        mContext = context;
        mMenuResourceId = menuResourceId;
        mAdapter = null;
        mHardwareButtonMenuAnchor = null;
        if (mContext instanceof Activity) {
            mHardwareButtonMenuAnchor = ((Activity)mContext).findViewById(R.id.menu_anchor_stub);
        }
    }

    public void addStat(int tabId, String block_type, String subresource) {
        if (!mTabsStat.containsKey(tabId)) {
            mTabsStat.put(tabId, new BlockersInfo());
        }
        BlockersInfo blockersInfo = mTabsStat.get(tabId);
        if (block_type.equals(BraveShieldsContentSettings.RESOURCE_IDENTIFIER_ADS)) {
            blockersInfo.mAdsBlocked++;
        } else if (block_type.equals(BraveShieldsContentSettings.RESOURCE_IDENTIFIER_TRACKERS)) {
            blockersInfo.mTrackersBlocked++;
        } else if (block_type.equals(BraveShieldsContentSettings.RESOURCE_IDENTIFIER_HTTP_UPGRADABLE_RESOURCES)) {
            blockersInfo.mHTTPSUpgrades++;
        } else if (block_type.equals(BraveShieldsContentSettings.RESOURCE_IDENTIFIER_JAVASCRIPTS)) {
            blockersInfo.mScriptsBlocked++;
        } else if (block_type.equals(BraveShieldsContentSettings.RESOURCE_IDENTIFIER_FINGERPRINTING)) {
            blockersInfo.mFingerprintsBlocked++;
        }
    }

    public void removeStat(int tabId) {
        if (!mTabsStat.containsKey(tabId)) {
            return;
        }
        mTabsStat.remove(tabId);
    }

    public void clearBraveShieldsCount(int tabId) {
        mTabsStat.put(tabId, new BlockersInfo());
    }

    public void addObserver(BraveShieldsMenuObserver menuObserver) {
        mMenuObserver = menuObserver;
    }

    public void show(View anchorView, String host, String title, int tabId,
            Profile profile) {
        if (mHardwareButtonMenuAnchor == null) return;
        int rotation = ((Activity)mContext).getWindowManager().getDefaultDisplay().getRotation();
        // This fixes the bug where the bottom of the menu starts at the top of
        // the keyboard, instead of overlapping the keyboard as it should.
        int displayHeight = mContext.getResources().getDisplayMetrics().heightPixels;
        int widthHeight = mContext.getResources().getDisplayMetrics().widthPixels;
        int currentDisplayWidth = widthHeight;

        // In appcompat 23.2.1, DisplayMetrics are not updated after rotation change. This is a
        // workaround for it. See crbug.com/599048.
        // TODO(ianwen): Remove the rotation check after we roll to 23.3.0.
        if (rotation == Surface.ROTATION_0 || rotation == Surface.ROTATION_180) {
            currentDisplayWidth = Math.min(displayHeight, widthHeight);
            displayHeight = Math.max(displayHeight, widthHeight);
        } else if (rotation == Surface.ROTATION_90 || rotation == Surface.ROTATION_270) {
            currentDisplayWidth = Math.max(displayHeight, widthHeight);
            displayHeight = Math.min(displayHeight, widthHeight);
        } else {
            assert false : "Rotation unexpected";
        }
        if (anchorView == null) {
            Rect rect = new Rect();
            ((Activity)mContext).getWindow().getDecorView().getWindowVisibleDisplayFrame(rect);
            int statusBarHeight = rect.top;
            mHardwareButtonMenuAnchor.setY((displayHeight - statusBarHeight));

            anchorView = mHardwareButtonMenuAnchor;
            //isByPermanentButton = true;
        }

        if (mMenu == null) {
            PopupMenu tempMenu = new PopupMenu(mContext, anchorView);
            tempMenu.inflate(mMenuResourceId);
            mMenu = tempMenu.getMenu();
        }
        ContextThemeWrapper wrapper = new ContextThemeWrapper(mContext, R.style.OverflowMenuThemeOverlay);
        Point pt = new Point();
        ((Activity)mContext).getWindowManager().getDefaultDisplay().getSize(pt);
        // Get the height and width of the display.
        Rect appRect = new Rect();
        ((Activity)mContext).getWindow().getDecorView().getWindowVisibleDisplayFrame(appRect);

        // Use full size of window for abnormal appRect.
        if (appRect.left < 0 && appRect.top < 0) {
            appRect.left = 0;
            appRect.top = 0;
            appRect.right = ((Activity)mContext).getWindow().getDecorView().getWidth();
            appRect.bottom = ((Activity)mContext).getWindow().getDecorView().getHeight();
        }

        mPopup = new ListPopupWindow(wrapper, null, android.R.attr.popupMenuStyle);
        mPopup.setModal(true);
        mPopup.setAnchorView(anchorView);
        mPopup.setInputMethodMode(PopupWindow.INPUT_METHOD_NOT_NEEDED);

        Drawable originalBgDrawable = mPopup.getBackground();

        mPopup.setAnimationStyle(R.style.OverflowMenuAnim);

        // Turn off window animations for low end devices, and on Android M, which has built-in menu
        // animations.
        if (SysUtils.isLowEndDevice() || Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            mPopup.setAnimationStyle(0);
        }

        Rect bgPadding = new Rect();
        mPopup.getBackground().getPadding(bgPadding);

        int popupWidth = wrapper.getResources().getDimensionPixelSize(R.dimen.menu_width)
                + bgPadding.left + bgPadding.right;

        // Extract visible items from the Menu.
        int numItems = mMenu.size();
        List<MenuItem> menuItems = new ArrayList<MenuItem>();
        int adsAndTrackers = 0;
        int httpsUpgrades = 0;
        int scriptsBlocked = 0;
        int fingerprintsBlocked = 0;
        if (mTabsStat.containsKey(tabId)) {
            BlockersInfo blockersInfo = mTabsStat.get(tabId);
            adsAndTrackers = blockersInfo.mAdsBlocked + blockersInfo.mTrackersBlocked;
            httpsUpgrades = blockersInfo.mHTTPSUpgrades;
            scriptsBlocked = blockersInfo.mScriptsBlocked;
            fingerprintsBlocked = blockersInfo.mFingerprintsBlocked;
        }
        for (int i = 0; i < numItems; ++i) {
            MenuItem item = mMenu.getItem(i);
            if (1 == i) {
                item.setTitle(title);
            } else if (3 == i) {
                item.setTitle(String.valueOf(adsAndTrackers));
            } else if (4 == i) {
                item.setTitle(String.valueOf(httpsUpgrades));
            } else if (5 == i) {
                item.setTitle(String.valueOf(scriptsBlocked));
            } else if (6 == i) {
                item.setTitle(String.valueOf(fingerprintsBlocked));
            }
            menuItems.add(item);
        }

        mPopup.setWidth(popupWidth);
        mPopup.setHeight(ListPopupWindow.WRAP_CONTENT);

        mAdapter = new BraveShieldsMenuAdapter(host, title, menuItems,
            LayoutInflater.from(wrapper), mMenuObserver, mPopup,
            currentDisplayWidth, profile);
        mPopup.setAdapter(mAdapter);

        mPopup.show();
        mPopup.getListView().setItemsCanFocus(true);

        // Don't animate the menu items for low end devices.
        if (!SysUtils.isLowEndDevice()) {
            mPopup.getListView().addOnLayoutChangeListener(new View.OnLayoutChangeListener() {
                @Override
                public void onLayoutChange(View v, int left, int top, int right, int bottom,
                        int oldLeft, int oldTop, int oldRight, int oldBottom) {
                    mPopup.getListView().removeOnLayoutChangeListener(this);
                    runMenuItemEnterAnimations();
                }
            });
        }
    }

    public void updateHost(String host) {
        if (mAdapter == null) {
            return;
        }

        mAdapter.updateHost(host);
    }

    private void runMenuItemEnterAnimations() {
        mMenuItemEnterAnimator = new AnimatorSet();
        AnimatorSet.Builder builder = null;

        ViewGroup list = mPopup.getListView();
        for (int i = 0; i < list.getChildCount(); i++) {
            View view = list.getChildAt(i);
            Object animatorObject = view.getTag(R.id.menu_item_enter_anim_id);
            if (animatorObject != null) {
                if (builder == null) {
                    builder = mMenuItemEnterAnimator.play((Animator) animatorObject);
                } else {
                    builder.with((Animator) animatorObject);
                }
            }
        }

        mMenuItemEnterAnimator.addListener(mAnimationHistogramRecorder);
        mMenuItemEnterAnimator.start();
    }

    public void updateValues(int tabId) {
        if (!mTabsStat.containsKey(tabId)) {
            return;
        }
        BlockersInfo blockersInfo = mTabsStat.get(tabId);
        updateValues(blockersInfo.mAdsBlocked + blockersInfo.mTrackersBlocked,
            blockersInfo.mHTTPSUpgrades, blockersInfo.mScriptsBlocked, blockersInfo.mFingerprintsBlocked);
    }

    public void updateValues(int adsAndTrackers, int httpsUpgrades, int scriptsBlocked, int fingerprintsBlocked) {
        final int fadsAndTrackers = adsAndTrackers;
        final int fhttpsUpgrades = httpsUpgrades;
        final int fscriptsBlocked = scriptsBlocked;
        final int ffingerprintsBlocked = fingerprintsBlocked;
        ((Activity)mContext).runOnUiThread(new Runnable() {
            @Override
            public void run() {
                if (!isShowing()) {
                    return;
                }
                try {
                    ViewGroup list = mPopup.getListView();
                    if (null == list || list.getChildCount() < 7) {
                        return;
                    }
                    for (int i = 0; i < list.getChildCount(); i++) {
                        View menuItemView = list.getChildAt(i);
                        if (null == menuItemView) {
                            continue;
                        }
                        TextView menuText = (TextView) menuItemView.findViewById(R.id.brave_shields_number);
                        if (null == menuText || null == menuText.getTag()) {
                            continue;
                        }
                        if ((int)menuText.getTag() == R.string.brave_shields_ads_and_trackers) {
                            // Set Ads and Trackers count
                            menuText.setText(String.valueOf(fadsAndTrackers));
                        } else if ((int)menuText.getTag() == R.string.brave_shields_https_upgrades) {
                            // Set HTTPS Upgrades count
                            menuText.setText(String.valueOf(fhttpsUpgrades));
                        } else if ((int)menuText.getTag() == R.string.brave_shields_scripts_blocked) {
                            // Set Scripts Blocked count
                            menuText.setText(String.valueOf(fscriptsBlocked));
                        } else if ((int)menuText.getTag() == R.string.brave_shields_fingerprint_methods) {
                            // Set Fingerprints Blocked count
                            menuText.setText(String.valueOf(ffingerprintsBlocked));
                        }
                    }
                }
                catch (NullPointerException exc) {
                    // It means that the Bravery Panel was destroyed during the update, we just do nothing
                }
            }
        });
    }

    public boolean isShowing() {
        if (null == mPopup) {
            return false;
        }

        return mPopup.isShowing();
    }

    public void hideBraveShieldsMenu() {
        if (isShowing()) {
            mPopup.dismiss();
            mAdapter = null;
        }
    }
}