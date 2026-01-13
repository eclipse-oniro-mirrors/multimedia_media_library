/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

var __decorate = this && this.__decorate || function (e, o, t, i) {
    var n, r = arguments.length, l = r < 3 ? o : null === i ? i = Object.getOwnPropertyDescriptor(o, t) : i;
    if ('object' === typeof Reflect && 'function' === typeof Reflect.decorate) {
        l = Reflect.decorate(e, o, t, i);
    } else {
        for (let s = e.length - 1; s >= 0; s--) {
            (n = e[s]) && (l = (r < 3 ? n(l) : r > 3 ? n(o, t, l) : n(o, t)) || l);
        }
    }
    return r > 3 && l && Object.defineProperty(o, t, l), l;
};
const fs = requireNapi('file.fs');
const fileUri = requireNapi('file.fileuri');
const bundleManager = requireNapi('bundle.bundleManager');
const LengthMetrics = requireNapi('arkui.node').LengthMetrics;
const LengthUnit = requireNapi('arkui.node').LengthUnit;
const photoAccessHelper = requireNapi('file.photoAccessHelper');
const PHOTO_VIEW_MIME_TYPE_MAP = new Map([
    ['*/*', 'FILTER_MEDIA_TYPE_ALL'],
    ['image/*', 'FILTER_MEDIA_TYPE_IMAGE'],
    ['video/*', 'FILTER_MEDIA_TYPE_VIDEO'],
    ['image/movingPhoto', 'FILTER_MEDIA_TYPE_IMAGE_MOVING_PHOTO']
]);
const display = requireNapi('display');
const cooperation_multi_name = ['Cooperation-multi', 'Cooperation'];

const OperationType = {
  EQUAL_TO : 1,
  NOT_EQUAL_TO : 2,
  GREATER_THAN : 3,
  LESS_THAN : 4,
  GREATER_THAN_OR_EQUAL_TO : 5,
  LESS_THAN_OR_EQUAL_TO : 6,
  AND : 7,
  OR : 8,
  IN : 9,
  NOT_IN : 10,
  BEGIN_WRAP : 11,
  END_WRAP : 12,
  BETWEEN : 13,
  NOT_BETWEEN : 14
}

const PickerFilterPhotoKeys = {
  
    URI: 'uri',
    
    PHOTO_TYPE: 'media_type',
    
    DISPLAY_NAME: 'display_name',
    
    SIZE: 'size',
    
    DURATION: 'duration',
    
    WIDTH: 'width',
    
    HEIGHT: 'height',
    
    ORIENTATION: 'orientation',
    
    FAVORITE: 'is_favorite',
    
    TITLE: 'title',
    
    POSITION: 'position',
    
    PHOTO_SUBTYPE: 'subtype',
    
    DYNAMIC_RANGE_TYPE: 'dynamic_range_type',
    
    COVER_POSITION: 'cover_position',
    
    BURST_KEY: 'burst_key',
    
    LCD_SIZE: 'lcd_size',
    
    THM_SIZE: 'thm_size',
    
    DETAIL_TIME: 'detail_time',
    
    OWNER_ALBUM_ID: 'owner_album_id',
    
    MEDIA_SUFFIX: 'media_suffix',
    
    ASPECT_RATIO: 'aspect_ratio',
  }


const PARAMETERS_VALIDATE_FAILED_MESSAGE = 
'Scene parameters validate failed, possible causes:' +
'  1. An invalid enumeration value was passed. Only MOVING_PHOTO_ENABLE and' +
'  MOVING_PHOTO_DISABLE are supported for configuration;';

const ILLEGAL_SCENARIO_CALL_ERROR_MESSAGE = 
'Invalid call context. Possible causes:' +
'  1. The API is called outside the photo browsing scenario.' +
'  2. The API is called when isMovingPhotoBadgeShown is already set to true.'

const PARAMETERS_VALIDATE_FAILED_CODE = 23800151;
const ILLEGAL_SCENARIO_CALL_ERROR_CODE = 23800202;
const PERMISSION_DENNIED = 401;

class BusinessError extends Error {
    constructor(msg, code) {
        super(msg);
        this.code = code || PERMISSION_DENNIED;
    }
}

export class PhotoPickerComponent extends ViewPU {
    constructor(e, o, t, i = -1, n = void 0) {
        super(e, t, i);
        'function' === typeof n && (this.paramsGenerator_ = n);
        this.pickerOptions = void 0;
        this.onSelect = void 0;
        this.onDeselect = void 0;
        this.onItemClicked = void 0;
        this.onItemClickedNotify = void 0;
        this.onEnterPhotoBrowser = void 0;
        this.onExitPhotoBrowser = void 0;
        this.onPhotoBrowserChangeStart = void 0;
        this.onError = void 0;
        this.onPickerControllerReady = void 0;
        this.onPhotoBrowserChanged = void 0;
        this.onSelectedItemsDeleted = void 0;
        this.onExceedMaxSelected = void 0;
        this.onCurrentAlbumDeleted = void 0;
        this.onVideoPlayStateChanged = void 0;
        this.onMovingPhotoBadgeStateChanged = void 0;
        this.badgeConfig = void 0;
        this.batchBadgeConfigSize = 1000;
        this.batchPreselectedInfos = 200;
        this.maxBadgeConfigSize = 200000;
        this.badgeConfigIsSending = void 0;
        this.preselectedInfos = void 0;
        this.onScrollStopAtStart = void 0;
        this.onScrollStopAtEnd = void 0;
        this.onPinchGridSwitched = void 0;
        this.__pickerController = new SynchedPropertyNesedObjectPU(o.pickerController, this, 'pickerController');
        this.proxy = void 0;
        this.dpiFollowStrategy = SecurityDpiFollowStrategy.FOLLOW_UI_EXTENSION_ABILITY_DPI;
        this.setInitiallyProvidedValue(o);
        this.pickerController.isPhotoBrowserShowCallback = ()=>{
            console.log('PhotoPickerComponent, isPhotoBrowserShowCallback init');
            return false;
        }
        this.pickerController.isMovingPhotoBadgeShownValidCallback = ()=>{
            console.log('PhotoPickerComponent, isMovingPhotoBadgeShownValidCallback init');
            return false;
        }
        this.declareWatch('pickerController', this.onChanged);
    }

    setInitiallyProvidedValue(e) {
        void 0 !== e.pickerOptions && (this.pickerOptions = e.pickerOptions);
        void 0 !== e.onSelect && (this.onSelect = e.onSelect);
        void 0 !== e.onDeselect && (this.onDeselect = e.onDeselect);
        void 0 !== e.onItemClicked && (this.onItemClicked = e.onItemClicked);
        void 0 !== e.onItemClickedNotify && (this.onItemClickedNotify = e.onItemClickedNotify);
        void 0 !== e.onEnterPhotoBrowser && (this.onEnterPhotoBrowser = e.onEnterPhotoBrowser);
        void 0 !== e.onExitPhotoBrowser && (this.onExitPhotoBrowser = e.onExitPhotoBrowser);
        void 0 !== e.onPhotoBrowserChanged && (this.onPhotoBrowserChanged = e.onPhotoBrowserChanged);
        void 0 !== e.onPickerControllerReady && (this.onPickerControllerReady = e.onPickerControllerReady);
        void 0 !== e.onSelectedItemsDeleted && (this.onSelectedItemsDeleted = e.onSelectedItemsDeleted);
        void 0 !== e.onExceedMaxSelected && (this.onExceedMaxSelected = e.onExceedMaxSelected);
        void 0 !== e.onCurrentAlbumDeleted && (this.onCurrentAlbumDeleted = e.onCurrentAlbumDeleted);
        void 0 !== e.onVideoPlayStateChanged && (this.onVideoPlayStateChanged = e.onVideoPlayStateChanged);
        void 0 !== e.onMovingPhotoBadgeStateChanged && (this.onMovingPhotoBadgeStateChanged = e.onMovingPhotoBadgeStateChanged);
        void 0 !== e.pickerOptions?.badgeConfig && (this.badgeConfig = e.pickerOptions?.badgeConfig);
        void 0 !== e.pickerOptions?.preselectedInfos && (this.preselectedInfos = e.pickerOptions?.preselectedInfos);
        void 0 !== e.onScrollStopAtStart && (this.onScrollStopAtStart = e.onScrollStopAtStart);
        void 0 !== e.onScrollStopAtEnd && (this.onScrollStopAtEnd = e.onScrollStopAtEnd);
        void 0 !== e.onPinchGridSwitched && (this.onPinchGridSwitched = e.onPinchGridSwitched);
        void 0 !== e.onPhotoBrowserChangeStart && (this.onPhotoBrowserChangeStart = e.onPhotoBrowserChangeStart);
        void 0 !== e.onError && (this.onError = e.onError);
        this.__pickerController.set(e.pickerController);
        if (this.badgeConfig && this.badgeConfig.uris !== undefined) {
            console.log('badgeConfig.uris.length:' + this.badgeConfig.uris.length);
            console.log('badgeConfig.badge_type:' + this.badgeConfig.badgeType);
            this.badgeConfig.uris.splice(this.maxBadgeConfigSize);
        }
        void 0 !== e.proxy && (this.proxy = e.proxy);
        const displayName = display.getDefaultDisplaySync().name;
        console.info(`displayName = ${displayName}`);
        if (cooperation_multi_name.includes(displayName)) {
            this.dpiFollowStrategy = SecurityDpiFollowStrategy.FOLLOW_HOST_DPI;
            console.info(`dpiFollowStrategy = ${this.dpiFollowStrategy}`);
        }
    }

    updateStateVars(e) {
        this.__pickerController.set(e.pickerController);
    }

    purgeVariableDependenciesOnElmtId(e) {
        this.__pickerController.purgeDependencyOnElmtId(e);
    }

    aboutToBeDeleted() {
        this.__pickerController.aboutToBeDeleted();
        SubscriberManager.Get().delete(this.id__());
        this.aboutToBeDeletedInternal();
    }

    get pickerController() {
        return this.__pickerController.get();
    }

    onChanged() {
        var e;
        if (!this.proxy) {
            return;
        }
        let o = null === (e = this.pickerController) || void 0 === e ? void 0 : e.data;
        if (null == o ? void 0 : o.has('SET_SELECTED_URIS')) {
            this.proxy.send({ selectUris: null == o ? void 0 : o.get('SET_SELECTED_URIS') });
            console.info('PhotoPickerComponent onChanged: SET_SELECTED_URIS');
        } else if (null == o ? void 0 : o.has('SET_ALBUM_URI')) {
            this.proxy.send({ albumUri: null == o ? void 0 : o.get('SET_ALBUM_URI') });
            console.info('PhotoPickerComponent onChanged: SET_ALBUM_URI');
        } else if (null == o ? void 0 : o.has('SET_SELECTED_INFO')) {
            this.onChangeSelectedInfo(o);
            console.info('PhotoPickerComponent onChanged: SET_SELECTED_INFO');
        } else if (null == o ? void 0 : o.has('SET_MAX_SELECT_COUNT')) {
            this.onSetMaxSelectCount(o);
        } else if (null == o ? void 0 : o.has('SET_PHOTO_BROWSER_ITEM')) {
            this.onSetPhotoBrowserItem(o);
        } else if (null == o ? void 0 : o.has('EXIT_PHOTO_BROWSER')) {
            this.handleExitPhotoBrowser();
        } else {
            this.otherOnChange(o);
        }
    }

    onChangedBadgeConfigs(o) {
        console.log('PhotoPickerComponent onChanged: onChangedBadgeConfigs');
        //上锁 只要还在发送中 就不做其他操作
        if (this.badgeConfigIsSending !== true) {
            console.log('badgeConfigType:' + o.get('BADGE_CONFIGS_OPTION_TYPE'));
            this.badgeConfig = o.get('SET_BADGE_CONFIGS');
            this.badgeConfigIsSending = true;
            this.badgeConfig.uris?.splice(this.maxBadgeConfigSize);
            this.proxy.send({needSendBadgeConfigs: true, BadgeOptionType: o.get('BADGE_CONFIGS_OPTION_TYPE')});
            console.log('badgeConfig.uris.length:' + this.badgeConfig?.uris?.length || '--');
        }
    }

    otherOnChange(o) {
        if (null == o ? void 0 : o.has('SET_PHOTO_BROWSER_UI_ELEMENT_VISIBILITY')) {
            this.onSetPhotoBrowserUIElementVisibility(o);
        } else if (null == o ? void 0 : o.has('CREATE_URI')) {
            this.onCreateUri(o);
            console.info('PhotoPickerComponent onChanged: CREATE_URI');
        } else if (null == o ? void 0 : o.has('REPLACE_URI')) {
            this.onReplaceUri(o);
            console.info('PhotoPickerComponent onChanged: REPLACE_URI');
        } else if (null == o ? void 0 : o.has('SAVE_REPLACE_PHOTO_ASSETS')) {
            this.onSaveTrustedPhotoAssets(o);
            console.info('PhotoPickerComponent onChanged: SAVE_REPLACE_PHOTO_ASSETS');
        } else if (null == o ? void 0 : o.has('SET_BADGE_CONFIGS')) {
            this.onChangedBadgeConfigs(o);
        } else if (null == o ? void 0 : o.has('UPDATE_CONFIG')) {
            this.onUpdateConfig(o);
        } else if (null == o ? void 0 : o.has('SET_ITEM_CLICK_RESULT')) {
            this.onChangeItemClickResult(o);
            console.info('PhotoPickerComponent onChanged: SET_ITEM_CLICK_RESULT');
        } else if (null == o ? void 0 : o.has('SET_MOVINGPHOTO_STATE')) {
            this.proxy.send({ movingPhotoState: null === o ? void 0 : o.get('SET_MOVINGPHOTO_STATE')});
            console.info('PhotoPickerComponent onChanged: SET_MOVINGPHOTO_STATE');
        } else {
            console.info('PhotoPickerComponent onChanged: other case');
        }
    }

    onChangeSelectedInfo(o) {
        this.preselectedInfos = null === o ? void 0 : o.get('SET_SELECTED_INFO');
        console.log(`preselectedInfos start send1 ${o.get('SET_SELECTED_INFO').slice(0, this.batchPreselectedInfos).length}`);
        this.proxy({ uriAndPickerIndexLists: null === o ? void 0 : o.get('SET_SELECTED_INFO').slice(0, this.batchPreselectedInfos),
             index: 0, preselectedInfosIsOver: this.preselectedInfos.length <= this.batchPreselectedInfos });
    }

    onChangeItemClickResult(o) {
        let itemClickResult = null === o ? void 0 : o.get('SET_ITEM_CLICK_RESULT');
        if(itemClickResult && itemClickResult instanceof Array && itemClickResult.length > 0) {
            this.proxy.send({ itemClickResult: itemClickResult });
            console.info(`PhotoPickerComponent send itemClickResult ${itemClickResult.length}`);
        }
    }

    onUpdateConfig(o) {
        this.proxy.send({ updateConfig: null == o ? void 0 : o.get('UPDATE_CONFIG') });
        console.info('PhotoPickerComponent onChanged: UPDATE_CONFIG');
    }

    onSetMaxSelectCount(o) {
        let e = null == o ? void 0 : o.get('SET_MAX_SELECT_COUNT');
        let t = null == e ? void 0 : e.data;
        this.proxy.send({
            totalCount: null == t ? void 0 : t.get(MaxCountType.TOTAL_MAX_COUNT),
            photoCount: null == t ? void 0 : t.get(MaxCountType.PHOTO_MAX_COUNT),
            videoCount: null == t ? void 0 : t.get(MaxCountType.VIDEO_MAX_COUNT)
        });
        console.info('PhotoPickerComponent onChanged: SET_MAX_SELECT_COUNT');
    }

    onSetPhotoBrowserItem(o) {
        let e = null == o ? void 0 : o.get('SET_PHOTO_BROWSER_ITEM');
        this.proxy.send({ 
            itemUri: null == e ? void 0 : e.uri, 
            photoBrowserRange: null == e ? void 0 : e.photoBrowserRange 
        });
        console.info('PhotoPickerComponent onChanged: SET_PHOTO_BROWSER_ITEM');
    }

    handleExitPhotoBrowser() {
        this.proxy.send({ exitPhotoBrowser: true });
        console.info('PhotoPickerComponent onChanged: EXIT_PHOTO_BROWSER');
    }

    onSetPhotoBrowserUIElementVisibility(o) {
        let e = null == o ? void 0 : o.get('SET_PHOTO_BROWSER_UI_ELEMENT_VISIBILITY');
        this.proxy.send({
            elements: null == e ? void 0 : e.elements,
            isVisible: null == e ? void 0 : e.isVisible
        });
        console.info('PhotoPickerComponent onChanged: SET_PHOTO_BROWSER_UI_ELEMENT_VISIBILITY');
    }

    onCreateUri(o) {
        let e = null == o ? void 0 : o.get('CREATE_URI');
        this.proxy.send({
            selectedMediaUri: null == e ? void 0 : e[0],
            createUri: null == e ? void 0 : e[1],
            date: null == e ? void 0 : e[2]
        });
        console.info('PhotoPickerComponent onChanged CREATE_URI ');
    }

    onReplaceUri(o) {
        let e = null == o ? void 0 : o.get('REPLACE_URI');
        this.proxy.send({
            oriUri: null == e ? void 0 : e[0],
            replaceUri: null == e ? void 0 : e[1],
            date: null == e ? void 0 : e[2]
        });
        console.info('PhotoPickerComponent onChanged REPLACE_URI');
    }

    onSaveTrustedPhotoAssets(o) {
        let e = null == o ? void 0 : o.get('SAVE_REPLACE_PHOTO_ASSETS');
        this.proxy.send({
            replaceUris: null == e ? void 0 : e[0],
            config: null == e ? void 0 : e[1],
            saveMode: null == e ? void 0 : e[2],
            appName: null == e ? void 0 : e[3],
            date: null == e ? void 0 : e[4]
        });
        console.info('PhotoPickerComponent onChanged SAVE_REPLACE_PHOTO_ASSETS');
    }

    checkAssetFilterInvalid(assetFilter) {
        // 获取所有有效的值
        const validOperationTypes = Object.values(OperationType);
        const validPhotoKeys = Object.values(PickerFilterPhotoKeys);

        //遍历数组中的每个OperationItem
        for (const item of assetFilter) {
            //检查operationType是否有值且在枚举中
            if (!item.operationType || !validOperationTypes.includes(item.operationType)) {
                console.log('PhotoPickerComponent, Invalid operationType');
                return true;
            }

            //如果field有值，检查是否在枚举中
            if (item.field !== undefined && item.field !== null) {
                if (!validPhotoKeys.includes(item.field)) {
                    console.log('PhotoPickerComponent, Invalid photokeys');
                    return true;
                }
                // uri仅支持EQUAL_TO操作
                if (item.field === PickerFilterPhotoKeys.URI && item.operationType !== OperationType.EQUAL_TO) {
                    console.log('PhotoPickerComponent, Invalid uri operation');
                    return true;
                }
            }
        }
        return false;
    }

    initialRender() {
        if (this.pickerOptions?.assetFilter && this.checkAssetFilterInvalid(this.pickerOptions.assetFilter)) {
            console.error('PhotoPickerComponent, assetFilter has value but invalid');
            return;
        }
        this.observeComponentCreation2(((e, o) => {
            Row.create();
            Row.height('100%');
        }), Row);
        this.observeComponentCreation2(((e, o) => {
            Column.create();
            Column.width('100%');
        }), Column);
        this.observeComponentCreation2(((e, o) => {
            var t, i, n, r, l, s, c, p, a, d, h, E, C, T, m, P, _, b, d, k, d, f, g, y, predicate;
            let z;
            let u;
            let state;
            SecurityUIExtensionComponent.create({
                parameters: {
                    'ability.want.params.uiExtensionTargetType': 'photoPicker',
                    uri: 'multipleselect',
                    targetPage: 'photoPage',
                    filterMediaType: this.convertMIMETypeToFilterType(null === (t = this.pickerOptions) || void 0 === t ? void 0 : t.MIMEType),
                    mimeTypeFilter: this.parseMimeTypeFilter(null === (i = this.pickerOptions) || void 0 === i ? void 0 : i.mimeTypeFilter),
                    fileSizeFilter: null === (i = this.pickerOptions) || void 0 === i ? void 0 : i.fileSizeFilter,
                    videoDurationFilter: null === (i = this.pickerOptions) || void 0 === i ? void 0 : i.videoDurationFilter,
                    photoViewMimeTypeFileSizeFilters: null === (i = this.pickerOptions) || void 0 === i ? void 0 : i.photoViewMimeTypeFileSizeFilters,
                    maxSelectNumber: null === (i = this.pickerOptions) || void 0 === i ? void 0 : i.maxSelectNumber,
                    isPhotoTakingSupported: null === (n = this.pickerOptions) || void 0 === n ? void 0 : n.isPhotoTakingSupported,
                    isEditSupported: !1,
                    recommendationOptions: null === (r = this.pickerOptions) || void 0 === r ? void 0 : r.recommendationOptions,
                    preselectedUri: null === (l = this.pickerOptions) || void 0 === l ? void 0 : l.preselectedUris,
                    isFromPickerView: !0,
                    isNeedActionBar: !1,
                    isNeedSelectBar: !1,
                    isSearchSupported: null === (s = this.pickerOptions) || void 0 === s ? void 0 : s.isSearchSupported,
                    checkBoxColor: null === (c = this.pickerOptions) || void 0 === c ? void 0 : c.checkBoxColor,
                    backgroundColor: null === (p = this.pickerOptions) || void 0 === p ? void 0 : p.backgroundColor,
                    checkboxTextColor: null === (a = this.pickerOptions) || void 0 === a ? void 0 : a.checkboxTextColor,
                    photoBrowserBackgroundColorMode: null === (d = this.pickerOptions) || void 0 === d ? void 0 : d.photoBrowserBackgroundColorMode,
                    isRepeatSelectSupported: null === (h = this.pickerOptions) || void 0 === h ? void 0 : h.isRepeatSelectSupported,
                    maxSelectedReminderMode: null === (E = this.pickerOptions) || void 0 === E ? void 0 : E.maxSelectedReminderMode,
                    orientation: null === (C = this.pickerOptions) || void 0 === C ? void 0 : C.orientation,
                    selectMode: null === (T = this.pickerOptions) || void 0 === T ? void 0 : T.selectMode,
                    maxPhotoSelectNumber: null === (m = this.pickerOptions) || void 0 === m ? void 0 : m.maxPhotoSelectNumber,
                    maxVideoSelectNumber: null === (P = this.pickerOptions) || void 0 === P ? void 0 : P.maxVideoSelectNumber,
                    isOnItemClickedSet: !!this.onItemClicked,
                    isOnItemClickedNotifySet: !!this.onItemClickedNotify,
                    isPreviewForSingleSelectionSupported: null === (_ = this.pickerOptions) || void 0 === _ ? void 0 : _.isPreviewForSingleSelectionSupported,
                    singleSelectionMode: null === (_ = this.pickerOptions) || void 0 === _ ? void 0 : _.singleSelectionMode,
                    isSlidingSelectionSupported: null === (b = this.pickerOptions) || void 0 === b ? void 0 : b.isSlidingSelectionSupported,
                    photoBrowserCheckboxPosition: null === (d = this.pickerOptions) || void 0 === d ? void 0 : d.photoBrowserCheckboxPosition,
                    gridMargin: null === (_ = this.pickerOptions) || void 0 === _ ? void 0 : _.gridMargin,
                    photoBrowserMargin: null === (_ = this.pickerOptions) || void 0 === _ ? void 0 : _.photoBrowserMargin,
                    gridStartOffset: null === (_ = this.pickerOptions) || void 0 === _ ? void 0 : _.gridStartOffset,
                    gridEndOffset: null === (_ = this.pickerOptions) || void 0 === _ ? void 0 : _.gridEndOffset,
                    singleLineConfig: null === (k = this.pickerOptions) || void 0 === k ? void 0 : this.getSingleLineConfig(k.singleLineConfig),
                    uiComponentColorMode: null === (d = this.pickerOptions) || void 0 === d ? void 0 : d.uiComponentColorMode,
                    combinedMediaTypeFilter: null === (f = this.pickerOptions) || void 0 === f ? void 0 : f.combinedMediaTypeFilter,
                    pickerIndex: null === (y = this.pickerOptions) || void 0 === y ? void 0 : y.pickerIndex,
                    isMovingPhotoBadgeShown:  this.parseIsMovingPhotoBadgeShown(null === (s = this.pickerOptions) || void 0 === s ? void 0 : s.isMovingPhotoBadgeShown),
                    isSlidingSupported:  null === (s = this.pickerOptions) || void 0 === s ? void 0 : s.isSlidingSupported,
                    assetFilter:  null === (predicate = this.pickerOptions) || void 0 === predicate ? void 0 : predicate.assetFilter,
                    edgeEffect: null === (s = this.pickerOptions) || void 0 === s ? void 0 : s.edgeEffect,
                    isOnScrollStopAtStartSet: !!this.onScrollStopAtStart,
                    isOnScrollStopAtEndSet: !!this.onScrollStopAtEnd,
                    autoPlayScenes: null === (z = this.pickerOptions) || void 0 === z ? void 0 : this.parseAutoPlayScenes(z.autoPlayScenes),
                    appAlbumFilters: null === (u = this.pickerOptions) || void 0 === u ? void 0 : this.parseAppAlbumFilters(u.appAlbumFilters),
                    gridPinchMode: null === (u = this.pickerOptions) || void 0 === u ? void 0 : u.gridPinchMode,
                    globalMovingPhotoState:  null === (state = this.pickerOptions) || void 0 === state ? void 0 : state.globalMovingPhotoState,
                }
            }
            ,{
                dpiFollowStrategy: this.dpiFollowStrategy
            });
            SecurityUIExtensionComponent.height('100%');
            SecurityUIExtensionComponent.width('100%');
            SecurityUIExtensionComponent.onRemoteReady((e => {
                this.proxy = e;
                console.info('PhotoPickerComponent onRemoteReady');
            }));
            SecurityUIExtensionComponent.onReceive((e => {
                let o = e;
                this.handleOnReceive(o);
            }));
            SecurityUIExtensionComponent.onError(((error) => {
                console.info('PhotoPickerComponent onError: ' + JSON.stringify(error));
            }));
        }), SecurityUIExtensionComponent);
        Column.pop();
        Row.pop();
    }

    handleOnReceive(e) {
        let o = e.dataType;
        console.info('PhotoPickerComponent onReceive: dataType = ' + o);
        if ('selectOrDeselect' === o) {
            this.handleSelectOrDeselect(e);
        } else if ('itemClick' === o) {
            this.handleItemClick(e); 
        } else if ('itemClickedNotify' === o) {
            this.handleItemClickedNotify(e);
        } else if ('onPhotoBrowserStateChanged' === o) {
            this.handleEnterOrExitPhotoBrowser(e);
        } else if ('remoteReady' === o) {
            if (this.onPickerControllerReady) {
                this.onPickerControllerReady();
                if (this.badgeConfig && this.badgeConfig?.uris?.length > 0) {
                    console.log('i need send msg to photo' + this.badgeConfig?.uris?.length + '---' +
                        this.badgeConfig?.badgeType + '---' + new Date().getTime().toString());
                    this.badgeConfigIsSending = true;
                    this.proxy.send({needSendBadgeConfigs: true, BadgeOptionType: BadgeOptionType.SET_DATA});
                }
                if (this.preselectedInfos && this.preselectedInfos.length > 0){
                    console.log('preselectedInfos start send');
                    this.proxy.send({ uriAndPickerIndexLists: this.preselectedInfos.slice(0, this.batchPreselectedInfos),
                        index: 0, preselectedInfosIsOver: this.preselectedInfos.length <= this.batchPreselectedInfos })
                }
                console.info('PhotoPickerComponent onReceive: onPickerControllerReady');
            }
        } else if ('replaceCallback' === o) {
            this.handleReplaceCallback(e);
        } else if ('createCallback' === o) {
            this.handleCreateCallback(e);
        } else if ('saveCallback' === o) {
            this.handleSaveCallback(e);
        } else if (dataType === 'onBackground') {
            console.info('PhotoPickerComponent onReceive: onBackground');
        } else if ('onPhotoBrowserChanged' === o) {
            this.handlePhotoBrowserChange(e);
        } else if ('onVideoPlayStateChanged' === o) {
            this.handleVideoPlayStateChanged(e);
        } else if ('onBadgeConfigSend' === o) {
            this.handleBadgeConfigSend(e);
        } else if ('onMovingPhotoBadgeStateChange' === o) {
            this.handleOnMovingPhotoBadgeStateChange(e);
        } else if ('onPhotoBrowserChangeStart' === o) {
            this.handleOnPhotoBrowserChangeStart(e);
        } else if ('onError' === o) {
            this.handleOnError(e);
        } else {
            this.handleOtherOnReceive(e);
            console.info('PhotoPickerComponent onReceive: other case');
        }
        console.info('PhotoPickerComponent onReceive' + this.pickerController.encrypt(JSON.stringify(e)));
    }

    handleBadgeConfigSend(e) {
        let index = e.nextIndex;
        if (this.badgeConfig && this.badgeConfig?.uris?.length > 0) {
            console.log('handleBadgeConfigSend uris.length: ' + this.badgeConfig.uris.length);
            const unitBadgeConfig = this.badgeConfig.uris.slice(this.batchBadgeConfigSize * index, this.batchBadgeConfigSize * (index + 1));
            console.log('index,unitunitBadgeConfig.length' + unitBadgeConfig?.length + 'index: ' + index +
                'this.badgeConfig.uris.length: ' + this.badgeConfig?.uris?.length
            );
            this.proxy.send({badgeConfig : unitBadgeConfig, index: index, 
                isOver : index >= Math.ceil(this.badgeConfig.uris.length / this.batchBadgeConfigSize) ? true : false, 
                badgeConfigType : this.badgeConfig.badgeType});
        }
        if (index >= Math.ceil(this.badgeConfig.uris.length / this.batchBadgeConfigSize)) {
            this.badgeConfigIsSending = false;
        }
    }

    handleOnMovingPhotoBadgeStateChange(e) {
        let uri = e.uri;
        let enabledMovingPhotoBadge = e.enabledMovingPhotoBadge;
        if (uri && enabledMovingPhotoBadge !== undefined && this.onMovingPhotoBadgeStateChanged) {
            this.onMovingPhotoBadgeStateChanged(uri, enabledMovingPhotoBadge);
        }
    }

    handleOtherOnReceive(e) {
        let o = e.dataType;
        if ('exceedMaxSelected' === o) {
            if (this.onExceedMaxSelected) {
                this.onExceedMaxSelected(e.maxCountType);
            }
        } else if ('selectedItemsDeleted' === o) {
            if (this.onSelectedItemsDeleted) {
                this.onSelectedItemsDeleted(e.selectedItemInfos);
            }
        } else if ('currentAlbumDeleted' === o) {
            if (this.onCurrentAlbumDeleted) {
                this.onCurrentAlbumDeleted();
            }
        } else if ('onPreselectedInfo' === o) {
            console.log(`send preselectUris isOver=${e.nextIndex >= Math.ceil(this.preselectedInfos.length / this.batchPreselectedInfos)}`);
            this.proxy.send({ uriAndPickerIndexLists: null == o ? void 0 :this.preselectedInfos.slice(e.nextIndex * this.batchPreselectedInfos, ( e.nextIndex + 1 ) * this.batchPreselectedInfos),
                index: e.nextIndex, preselectedInfosIsOver: e.nextIndex >= Math.ceil(this.preselectedInfos.length / this.batchPreselectedInfos)})
        } else if ('onScrollStopAtStart' === o) {
            if (this.onScrollStopAtStart) {
                this.onScrollStopAtStart();
            }
        } else if ('onScrollStopAtEnd' === o) {
            if (this.onScrollStopAtEnd) {
                this.onScrollStopAtEnd();
            }
        } else if ('onGridLevelChanged' === o) {
            if (this.onPinchGridSwitched) {
                this.onPinchGridSwitched(e.gridLevel);
            }
        } else {
            console.info('PhotoPickerComponent onReceive: other case');
        }
    }

    handleSelectOrDeselect(e) {
        if (e.isSelect) {
            if (this.onSelect) {
                this.onSelect(e['select-item-list']);
                console.info('PhotoPickerComponent onReceive: onSelect');
            }
        } else if (this.onDeselect) {
            this.onDeselect(e['select-item-list']);
            console.info('PhotoPickerComponent onReceive: onDeselect');
        }
    }

    handleItemClick(e) {
        if (this.onItemClicked) {
            let o = this.getClickType(e);
            let n = e.itemType;
            let i = this.getItemInfo(e);
            let r = this.onItemClicked(i, o);
            console.info('PhotoPickerComponent onReceive: onItemClicked = ' + o);
            if (this.proxy) {
                if ('thumbnail' === n && o === ClickType.SELECTED) {
                    this.proxy.send({ clickConfirm: i.uri, isConfirm: r });
                    console.info('PhotoPickerComponent onReceive: click confirm: uri = ' +
                        this.pickerController.encrypt(i.uri) + 'isConfirm = ' + r);
                }
                if ('camera' === n) {
                    this.proxy.send({ enterCamera: r });
                    console.info('PhotoPickerComponent onReceive: enter camera ' + r);
                }
            }
        }
    }

    handleItemClickedNotify(e) {
        if (this.onItemClickedNotify) {
            let o = this.getClickType(e);
            let i = this.getItemInfo(e);
            this.onItemClickedNotify(i, o);
            console.info('PhotoPickerComponent onReceive: onItemClickedNotify = ' + o);
        }
    }

    getClickType(e) {
        let o = ClickType.SELECTED;
        let t = e.clickType;
        'select' === t ? o = ClickType.SELECTED : 'deselect' === t ? o = ClickType.DESELECTED : console.info('PhotoPickerComponent onReceive: other clickType');
        return o;
    }

    getItemInfo(e) {
        let i = new ItemInfo;
        let n = e.itemType;
        'thumbnail' === n ? i.itemType = ItemType.THUMBNAIL : 'camera' === n ? i.itemType = ItemType.CAMERA : console.info('PhotoPickerComponent onReceive: other itemType');
        i.uri = e.uri;
        i.mimeType = e.mimeType;
        i.width = e.width;
        i.height = e.height;
        i.size = e.size;
        i.duration = e.duration;
        i.photoSubType = e.subtype;
        i.dynamicRangeType = e.dynamicRangeType;
        i.orientation = e.imageOrientation;
        i.movingPhotoBadgeState = e.movingPhotoBadgeState;
        i.videoMode = e.videoMode;
        return i;
    }

    handleEnterOrExitPhotoBrowser(e) {
        let o = e.isEnter;
        let t = new PhotoBrowserInfo;
        t.animatorParams = new AnimatorParams;
        t.animatorParams.duration = e.duration;
        t.animatorParams.curve = e.curve;
        o ? this.onEnterPhotoBrowser && this.onEnterPhotoBrowser(t) : this.onExitPhotoBrowser && this.onExitPhotoBrowser(t);
        this.pickerController.isPhotoBrowserShowCallback = () =>{
            if (o) {
                return true;
            } else {
                return false;
            }
        }
        console.info('PhotoPickerComponent onReceive: onPhotoBrowserStateChanged = ' + o);
    }

    handlePhotoBrowserChange(e) {
        let o = new BaseItemInfo();
        o.uri = e.uri;
        o.photoSubType = e.photoSubType;
        o.mimeType = e.mimeType;
        if (this.onPhotoBrowserChanged) {
            this.onPhotoBrowserChanged(o);
        }
        console.info('PhotoPickerComponent onReceive: onPhotoBrowserChanged = ' + this.pickerController.encrypt(o.uri));
    }

    handleOnPhotoBrowserChangeStart(e) {
        let o = new BaseItemInfo();
        o.uri = e.uri;
        o.photoSubType = e.photoSubType;
        o.movingPhotoBadgeState = e.movingPhotoBadgeState;
        if (this.onPhotoBrowserChangeStart) {
            this.onPhotoBrowserChangeStart(o);
        }
        console.info('PhotoPickerComponent onReceive: onPhotoBrowserChangeStart = ' 
            + this.pickerController.encrypt(o.uri));
    }

    handleOnError(e) {
        let pickerError = new PickerError();
        pickerError.functionName = e.functionName;
        pickerError.errorCode = e.errorCode;
        pickerError.message = e.errorMessage;
        console.info('PhotoPickerComponent onReceive: onError: ' 
            + JSON.stringify(e));
        if (this.onError) {
            this.onError(pickerError);
        }
        console.info('PhotoPickerComponent onReceive: onError: pickerError ' 
            + JSON.stringify(pickerError));
    }

    handleVideoPlayStateChanged(e) {
        if (this.onVideoPlayStateChanged) {
            this.onVideoPlayStateChanged(e.state);
            console.info('PhotoPickerComponent onReceive: onVideoPlayStateChanged = ' + JSON.stringify(e));
        }
    }

    handleCreateCallback(e) {
        this.pickerController.actionCreateCallback(e.grantUri, e.date, e.code, e.message);
        console.info('PhotoPickerComponent onReceive: handleCreateCallback');
    }

    handleReplaceCallback(e) {
        this.pickerController.actionReplaceCallback(e.date, {'name': '', 'code': e.code, 'message': e.message});
        console.info('PhotoPickerComponent onReceive: handleReplaceCallback');
    }

    handleSaveCallback(e) {
        this.pickerController.actionSaveCallback(e.date, {'name': '', 'code': e.code, 'message': e.message}, e.data);
        console.info('PhotoPickerComponent onReceive: handleSaveCallback');
    }

    parseAutoPlayScenes(autoPlayScenes) {
        if (!autoPlayScenes) {
            return undefined;
        }

        if (autoPlayScenes.length > 2) {
            return autoPlayScenes.slice(0, 2);
        }
        
        return autoPlayScenes;
    }
    
    parseAppAlbumFilters(appAlbumFilters) {
        if (!appAlbumFilters) {
            return undefined;
        }

        if (appAlbumFilters.length > 3) {
            return appAlbumFilters.slice(0, 3);
        }

        return appAlbumFilters;
    }   
    
    parseIsMovingPhotoBadgeShown(isMovingPhotoBadgeShown) {
        console.log('isMovingPhotoBadgeShown=' + isMovingPhotoBadgeShown);
        if (isMovingPhotoBadgeShown === undefined || void 0 === isMovingPhotoBadgeShown) {
            return undefined;
        }
        this.pickerController.isMovingPhotoBadgeShownValidCallback = ()=>{
            if (isMovingPhotoBadgeShown === true) {
                return true;
            } else {
                return false;
            }
        }
        return isMovingPhotoBadgeShown;
        
    }

    parseMimeTypeFilter(filter) {
        if (!filter) {
            return undefined;
        }
        let o = {};
        o.mimeTypeArray = [];
        if (filter.mimeTypeArray) {
            for (let mimeType of filter.mimeTypeArray) {
                if (PHOTO_VIEW_MIME_TYPE_MAP.has(mimeType)) {
                    o.mimeTypeArray.push(PHOTO_VIEW_MIME_TYPE_MAP.get(mimeType));
                  } else {
                    o.mimeTypeArray.push(mimeType);
                }
            }
        }
        return o;
    }

    convertMIMETypeToFilterType(e) {
        let o;
        if (PHOTO_VIEW_MIME_TYPE_MAP.has(e)) {
            o = PHOTO_VIEW_MIME_TYPE_MAP.get(e);
        } else {
            o = PHOTO_VIEW_MIME_TYPE_MAP.get('*/*');
        }
        console.info('PhotoPickerComponent convertMIMETypeToFilterType: ' + JSON.stringify(o));
        return o;
    }

    getSingleLineConfig(singleLineConfig) {
        if (null === singleLineConfig || void 0 === singleLineConfig) {
            return void 0;
        }
        singleLineConfig.itemDisplayRatio =
            (null === singleLineConfig.itemDisplayRatio || void 0 === singleLineConfig.itemDisplayRatio) ?
            ItemDisplayRatio.SQUARE_RATIO : singleLineConfig.itemDisplayRatio;
        singleLineConfig.itemBorderRadius =
            this.getSingleLineConfigItemBorderRadius(singleLineConfig.itemBorderRadius);
        singleLineConfig.itemGap = this.getLength(singleLineConfig.itemGap);
        return singleLineConfig;
    }

    getSingleLineConfigItemBorderRadius(itemBorderRadius) {
        if (null === itemBorderRadius || void 0 === itemBorderRadius) {
            return 0;
        }
        if (typeof itemBorderRadius === 'number' || typeof itemBorderRadius === 'string') {
            return itemBorderRadius;
        }
        if (this.hasOwnProp(itemBorderRadius, ['topStart', 'topEnd', 'bottomStart', 'bottomEnd'])) {
            const l2 = {
                topStart: LengthMetrics.vp(0),
                topEnd: LengthMetrics.vp(0),
                bottomStart: LengthMetrics.vp(0),
                bottomEnd: LengthMetrics.vp(0),
            };
            const m2 = itemBorderRadius;
            l2.topStart = m2.topStart ? m2.topStart : LengthMetrics.vp(0);
            l2.topEnd = m2.topEnd ? m2.topEnd : LengthMetrics.vp(0);
            l2.bottomStart = m2.bottomStart ? m2.bottomStart : LengthMetrics.vp(0);
            l2.bottomEnd = m2.bottomEnd ? m2.bottomEnd : LengthMetrics.vp(0);
            return l2;
        }
        if (this.hasOwnProp(itemBorderRadius, ['topLeft', 'topRight', 'bottomLeft', 'bottomRight'])) {
            const borderRadiuses = {
                topLeft: 0,
                topRight: 0,
                bottomLeft: 0,
                bottomRight: 0
            };
            const k2 = itemBorderRadius;
            borderRadiuses.topLeft = this.getLength(k2.topLeft);
            borderRadiuses.topRight = this.getLength(k2.topRight);
            borderRadiuses.bottomLeft = this.getLength(k2.bottomLeft);
            borderRadiuses.bottomRight = this.getLength(k2.bottomRight);
            return borderRadiuses;
        }
        const j2 = itemBorderRadius;
        const resource = LengthMetrics.resource(j2);
        if (M.getInstance().isValid(resource)) {
            return M.getInstance().stringify(resource);
        }
        return 0;
    }

    getLength(prop) {
        if (null === prop || void 0 === prop) {
            return 0;
        }
        if (typeof prop === 'number' || typeof prop === 'string') {
            return prop;
        }
        const resource = LengthMetrics.resource(prop);
        if (M.getInstance().isValid(resource)) {
            return M.getInstance().stringify(resource);
        }
        return 0;
    }

    hasOwnProp(obj, props) {
        for (const key of Object.keys(obj)) {
            if (props.includes(key)) {
                return true;
            }
        }
        return false;
    }

    rerender() {
        this.updateDirtyElements();
    }
}

class M {
    constructor() {
    }

    static getInstance() {
        if (!M.instance) {
            M.instance = new M();
        }
        return M.instance;
    }

    stringify(metrics) {
        if (null === metrics || void 0 === metrics || typeof metrics !== 'object' || null === metrics.unit ||
            void 0 === metrics.unit || null === metrics.value || void 0 === metrics.value) {
            return '0vp';
        }
        switch (metrics.unit) {
            case LengthUnit.PX:
                return `${metrics.value}px`;
            case LengthUnit.VP:
                return `${metrics.value}vp`;
            case LengthUnit.FP:
                return `${metrics.value}fp`;
            case LengthUnit.PERCENT:
                return `${metrics.value}%`;
            case LengthUnit.LPX:
                return `${metrics.value}lpx`;
            default:
                return '0vp';
        }
    }

    isValid(metrics) {
        if (null === metrics || void 0 === metrics || typeof metrics !== 'object' ||
            null === metrics.value || void 0 === metrics.value) {
            return false;
        }
        return metrics.value > 0;
    }
}

let PickerController = class {
    constructor() {
        this.replaceCallbackMap = new Map();
        this.saveCallbackMap = new Map();
        this.createCallbackMap = new Map();
        this.saveCallbackPromises = new Map();
        this.isPhotoBrowserShowCallback = function() {
        }; // 用于监听是否进大图浏览，true表示进入，false表示推出。
        this.isMovingPhotoBadgeShownValidCallback = function() {
        }; // 用于监听是否配置了显示动图照片角标（isMovingPhotoBadgeShown）。true表示显示，false表示不显示。
    }
    setData(e, o) {
        if (o === undefined) {
            return;
        }
        if (e === DataType.SET_SELECTED_URIS) {
            if (o instanceof Array) {
                let e = o;
                if (e) {
                    this.data = new Map([['SET_SELECTED_URIS', [...e]]]);
                    console.info('PhotoPickerComponent SET_SELECTED_URIS' + this.encrypt(JSON.stringify(e)));
                }
            }
        } else if (e === DataType.SET_ALBUM_URI) {
            let e = o;
            if (e !== undefined) {
                this.data = new Map([['SET_ALBUM_URI', e]]);
                console.info('PhotoPickerComponent SET_ALBUM_URI' + this.encrypt(JSON.stringify(e)));
            }
        } else if (e === DataType.SET_BADGE_CONFIGS) {
            let e = o;
            if (e !== undefined) {
                this.data = new Map([['SET_BADGE_CONFIGS', e], ['BADGE_CONFIGS_OPTION_TYPE', BadgeOptionType.SET_DATA]]);
                console.info('PhotoPickerComponent SET_BADGE_CONFIGS set_data' + this.encrypt(JSON.stringify(e)));
            }
        } else if (e === DataType.SET_SELECTED_INFO) {
            if (o instanceof Array) {
                let e = o;
                if (e) {
                    this.data = new Map([['SET_SELECTED_INFO', [...e]]]);
                    console.info('PhotoPickerComponent SET_SELECTED_INFO' + this.encrypt(JSON.stringify(e)));
                }
            }
        } else {
            console.info('PhotoPickerComponent setData: other case');
        } 
    }

    addData(e, o) {
        if (o === undefined) {
            return;
        }
        if (e === DataType.SET_BADGE_CONFIGS) {
            let e = o;
            if (e !== undefined) {
                this.data = new Map([['SET_BADGE_CONFIGS', e], ['BADGE_CONFIGS_OPTION_TYPE', BadgeOptionType.ADD_DATA]]);
                console.info('PhotoPickerComponent SET_BADGE_CONFIGS add_data' + this.encrypt(JSON.stringify(e)));
            }
            return;
        } else if (e === DataType.SET_ITEM_CLICK_RESULT) {
            if (o instanceof Array) {
                let e = o;
                if (e) {
                    this.data = new Map([['SET_ITEM_CLICK_RESULT', [...e]]]);
                    console.info('PhotoPickerComponent SET_ITEM_CLICK_RESULT' + this.encrypt(JSON.stringify(e)));
                }
            }
        }
    }

    deleteData(e, o) {
        if (o === undefined) {
            return;
        }
        if (e === DataType.SET_BADGE_CONFIGS) {
            let e = o;
            if (e !== undefined) {
                this.data = new Map([['SET_BADGE_CONFIGS', e], ['BADGE_CONFIGS_OPTION_TYPE', BadgeOptionType.DELETE_DATA]]);
                console.info('PhotoPickerComponent SET_BADGE_CONFIGS delete_data' + this.encrypt(JSON.stringify(e)));

            }
            return;
        }
    }

    async setMovingPhotoState(e) {
        if (e !== MovingPhotoBadgeStateType.ADD_DATA && e !== MovingPhotoBadgeStateType.DELETE_DATA) {
            throw new BusinessError(PARAMETERS_VALIDATE_FAILED_MESSAGE, PARAMETERS_VALIDATE_FAILED_CODE);
        }
        let isPhotoBrowserShow = this?.isPhotoBrowserShowCallback();
        let isMovingPhotoBadgeShownValid = this?.isMovingPhotoBadgeShownValidCallback();
        console.info('SET_MOVINGPHOTO_STATE, this.isPhotoBrowserShow : ' + JSON.stringify(isPhotoBrowserShow) +
         ', this.isMovingPhotoBadgeShownValid=' + isMovingPhotoBadgeShownValid);
        if (!isPhotoBrowserShow || isMovingPhotoBadgeShownValid) {
            throw new BusinessError(ILLEGAL_SCENARIO_CALL_ERROR_MESSAGE, ILLEGAL_SCENARIO_CALL_ERROR_CODE);
        }
        if (e !== undefined) {
            this.data = new Map([['SET_MOVINGPHOTO_STATE', e]]);
            console.info('PhotoPickerComponent SET_MOVINGPHOTO_STATE: ' + this.encrypt(JSON.stringify(e)));
        }
    }

    setMaxSelected(e) {
        if (e) {
            this.data = new Map([['SET_MAX_SELECT_COUNT', e]]);
            console.info('PhotoPickerComponent SET_MAX_SELECT_COUNT' + JSON.stringify(e));
        }
    }

    async updatePickerOptions(e) {
        if (e !== undefined) {
            this.data = new Map([['UPDATE_CONFIG', e]]);
            console.info('PhotoPickerComponent UPDATE_CONFIG ' + JSON.stringify(e));
        }
    }
    
    setPhotoBrowserItem(e, o) {
        let l = new PhotoBrowserRangeInfo;
        l.uri = e;
        let m = o ? o : PhotoBrowserRange.ALL;
        l.photoBrowserRange = m;
        this.data = new Map([['SET_PHOTO_BROWSER_ITEM', l]]);
        console.info('PhotoPickerComponent SET_PHOTO_BROWSER_ITEM ' + this.encrypt(JSON.stringify(l)));
    }

    exitPhotoBrowser() {
        this.data = new Map([['EXIT_PHOTO_BROWSER', true]]);
        console.info('PhotoPickerComponent EXIT_PHOTO_BROWSER ');
    }

    async getAppName() {
        let flags = bundleManager.BundleFlag.GET_BUNDLE_INFO_WITH_ABILITY |
            bundleManager.BundleFlag.GET_BUNDLE_INFO_WITH_HAP_MODULE |
            bundleManager.BundleFlag.GET_BUNDLE_INFO_WITH_SIGNATURE_INFO |
            bundleManager.BundleFlag.GET_BUNDLE_INFO_WITH_APPLICATION;
        let c2 = bundleManager.getBundleInfoForSelfSync(flags);
        let labelId = c2.appInfo.labelId;
        let appName = '';
        let moduleName = '';
        for (let d2 of c2.hapModulesInfo) {
            if (labelId === d2.labelId) {
                moduleName = d2.name;
            }
        }
        appName = await getContext(this).createModuleContext(moduleName).resourceManager.getStringValue(labelId);
        return appName;
    }

    replacePhotoPickerPreview(e, o, callback) {
        try {
            let fd = fs.openSync(o).fd;
            fs.close(fd);
        } catch (err) {
            callback({'code': 13900002, 'message': 'No such file', name: ''});
            return;
        }
        let date = Math.random();
        this.data = new Map([['CREATE_URI', [e, o, date]]]);
        this.createCallbackMap.set(date, (grantUri, code, message)=>{
            if (code !== 0) {
                callback({'code': code, 'message': message, name: ''});
                return;
            }
            let createFd = 0;
            let replaceFd = 0;
            try {
                createFd = fs.openSync(grantUri, fs.OpenMode.READ_WRITE).fd;
                replaceFd = fs.openSync(o, fs.OpenMode.READ_ONLY).fd;
                fs.copyFileSync(replaceFd, createFd);
                this.data = new Map([['REPLACE_URI', [e, grantUri, date]]]);
                this.replaceCallbackMap.set(date, callback);
            } catch (err) {
                callback({'code': 14000011, 'message': 'System inner fail', name: ''});
            } finally {
                fs.close(createFd);
                fs.close(replaceFd);
            }
        });
    }

    saveTrustedPhotoAssets(e, callback, config, saveMode) {
        if (!e || e.length === 0) {
            callback({'code': 14000002, 'message': 'Invalid URI', name: ''}, []);
            return;
        }
        this.getAppName().then((appName) => {
            let date = Math.random();
            this.data = new Map([['SAVE_REPLACE_PHOTO_ASSETS', [e, config, saveMode, appName, date]]]);
            this.saveCallbackMap.set(date, callback);
            console.info('PhotoPickerComponent SAVE_TRUSTED_PHOTO_ASSETS ');
        });
    }

    async saveTrustedPhotoAssetsEx(e, settings, saveMode) {
        return new Promise((resolve, reject) => {
            if (!e || e.length === 0) {
                reject({'code': 14000002, 'message': 'Invalid URI', name: ''}, []);
                return;
            }
            this.getAppName().then((appName) => {
                let date = Math.random();
                this.data = new Map([['SAVE_REPLACE_PHOTO_ASSETS', [e, settings, saveMode, appName, date]]]);
                this.saveCallbackPromises.set(date, [resolve, reject]);
                console.info('PhotoPickerComponent SAVE_TRUSTED_PHOTO_ASSETS_EX');
            });
        });
    }

    actionCreateCallback(grantUri, date, code, message) {
        if (this.createCallbackMap.has(date)) {
            let callback = this.createCallbackMap.get(date);
            if (callback) {
                callback(grantUri, code, message);
                this.createCallbackMap.delete(date);
            }
        }
    }

    actionReplaceCallback(date, err) {
        if (this.replaceCallbackMap.has(date)) {
            let callback = this.replaceCallbackMap.get(date);
            if (callback) {
                callback(err);
                this.replaceCallbackMap.delete(date);
            }
        }
    }
    
    actionSaveCallback(date, err, data) {
        if (this.saveCallbackMap.has(date)) {
            let callback = this.saveCallbackMap.get(date);
            if (callback) {
                callback(err, data);
                this.saveCallbackMap.delete(date);
            }
        }
        if (this.saveCallbackPromises.has(date)) {
            let promiseFunc = this.saveCallbackMap.get(date);
            if (promiseFunc) {
              if (err) {
                promiseFunc[1](err);
              } else {
                promiseFunc[0](data);
              }
              this.saveCallbackPromises.delete(date);
            }
        }
    }

    setPhotoBrowserUIElementVisibility(e, o) {
        let m = new PhotoBrowserUIElementVisibility;
        m.elements = e;
        m.isVisible = o;
        this.data = new Map([['SET_PHOTO_BROWSER_UI_ELEMENT_VISIBILITY', m]]);
        console.info('PhotoPickerComponent SET_PHOTO_BROWSER_UI_ELEMENT_VISIBILITY ' + JSON.stringify(m));
    }

    encrypt(data) {
        if (!data || data?.indexOf('file:///data/storage/') !== -1) {
          return '';
        }
        return data.replace(/(\/\w+)\./g, '/******.');
    }
};
PickerController = __decorate([Observed], PickerController);

export class GridPinchMode {
    constructor() {
        this.gridPinchModeType = undefined;
        this.defaultGridLevel = GridLevel.STANDARD;
    }
}

export class PickerOptions extends photoAccessHelper.BaseSelectOptions {
}

export class UpdatablePickerConfigs {    
}

export class BaseItemInfo {
}

export class ItemInfo extends BaseItemInfo {
}

export class PhotoBrowserInfo {
}

export class AnimatorParams {
}

export class MaxSelected {
}

export class PickerError {

}

class PhotoBrowserRangeInfo {
}

class PhotoBrowserUIElementVisibility {
}

class PreselectedInfo {    
}

class ClickResult {    
}

export class SingleLineConfig {
    constructor() {
        this.itemDisplayRatio = ItemDisplayRatio.SQUARE_RATIO;
        this.itemBorderRadius = 0;
        this.itemGap = 0;
    }
}

export class BadgeConfig {

}

export var GridPinchModeType;
!function(e) {
    e[e.FULL_FUNCTION_GRID = 0] = 'FULL_FUNCTION_GRID';
}(GridPinchModeType || (GridPinchModeType = {}));

export var GridLevel;
!function(e) {
    e[e.SPACIOUS = 0] = 'SPACIOUS';
    e[e.STANDARD = 1] = 'STANDARD';
    e[e.COMPACT = 2] = 'COMPACT';
}(GridLevel || (GridLevel = {}));

export var DataType;
!function(e) {
    e[e.SET_SELECTED_URIS = 1] = 'SET_SELECTED_URIS';
    e[e.SET_ALBUM_URI = 2] = 'SET_ALBUM_URI';
    e[e.SET_SELECTED_INFO = 3] = 'SET_SELECTED_INFO';
    e[e.SET_BADGE_CONFIGS = 4] = 'SET_BADGE_CONFIGS';
    e[e.SET_ITEM_CLICK_RESULT = 5] = 'SET_ITEM_CLICK_RESULT';
}(DataType || (DataType = {}));

export var ItemType;
!function(e) {
    e[e.THUMBNAIL = 0] = 'THUMBNAIL';
    e[e.CAMERA = 1] = 'CAMERA';
}(ItemType || (ItemType = {}));

export var ClickType;
!function(e) {
    e[e.SELECTED = 0] = 'SELECTED';
    e[e.DESELECTED = 1] = 'DESELECTED';
}(ClickType || (ClickType = {}));

export var PickerOrientation;
!function(e) {
    e[e.VERTICAL = 0] = 'VERTICAL';
    e[e.HORIZONTAL = 1] = 'HORIZONTAL';
}(PickerOrientation || (PickerOrientation = {}));

export var SelectMode;
!function(e) {
    e[e.SINGLE_SELECT = 0] = 'SINGLE_SELECT';
    e[e.MULTI_SELECT = 1] = 'MULTI_SELECT';
}(SelectMode || (SelectMode = {}));

export var PickerColorMode;
!function(e) {
    e[e.AUTO = 0] = 'AUTO';
    e[e.LIGHT = 1] = 'LIGHT';
    e[e.DARK = 2] = 'DARK';
}(PickerColorMode || (PickerColorMode = {}));

export var ReminderMode;
!function(e) {
    e[e.NONE = 0] = 'NONE';
    e[e.TOAST = 1] = 'TOAST';
    e[e.MASK = 2] = 'MASK';
}(ReminderMode || (ReminderMode = {}));

export var MaxCountType;
!function(e) {
    e[e.TOTAL_MAX_COUNT = 0] = 'TOTAL_MAX_COUNT';
    e[e.PHOTO_MAX_COUNT = 1] = 'PHOTO_MAX_COUNT';
    e[e.VIDEO_MAX_COUNT = 2] = 'VIDEO_MAX_COUNT';
}(MaxCountType || (MaxCountType = {}));

export var PhotoBrowserRange;
!function(e) {
    e[e.ALL = 0] = 'ALL';
    e[e.SELECTED_ONLY = 1] = 'SELECTED_ONLY';
}(PhotoBrowserRange || (PhotoBrowserRange = {}));

export var PhotoBrowserUIElement;
!function(e) {
    e[e.CHECKBOX = 0] = 'CHECKBOX';
    e[e.BACK_BUTTON = 1] = 'BACK_BUTTON';
}(PhotoBrowserUIElement || (PhotoBrowserUIElement = {}));

export var VideoPlayerState;
!function(e) {
    e[e.PLAYING = 0] = 'PLAYING';
    e[e.PAUSED = 1] = 'PAUSED';
    e[e.STOPPED = 2] = 'STOPPED';
    e[e.SEEK_START = 3] = 'SEEK_START';
    e[e.SEEK_FINISH = 4] = 'SEEK_FINISH';
}(VideoPlayerState || (VideoPlayerState = {}));

export var SaveMode;
!function(e) {
    e[e.SAVE_AS = 0] = 'SAVE_AS';
    e[e.OVERWRITE = 1] = 'OVERWRITE';
}(SaveMode || (SaveMode = {}));

export var ItemDisplayRatio;
!function(e) {
    e[e.SQUARE_RATIO = 0] = 'SQUARE_RATIO';
    e[e.ORIGINAL_SIZE_RATIO = 1] = 'ORIGINAL_SIZE_RATIO';
}(ItemDisplayRatio || (ItemDisplayRatio = {}));

export var BadgeType;
!function(e) {
    e[e.BADGE_UPLOADED = 1] = 'BADGE_UPLOADED';
}(BadgeType || (BadgeType = {}));

export var BadgeOptionType;
!function(e) {
    e[e.SET_DATA = 1] = 'SET_DATA';
    e[e.ADD_DATA = 2] = 'ADD_DATA';
    e[e.DELETE_DATA = 3] = 'DELETE_DATA';
}(BadgeOptionType || (BadgeOptionType = {}));

export var MovingPhotoBadgeStateType;
!function(e) {
    e[e.SET_DATA = 0] = 'NOT_MOVING_PHOTO';
    e[e.ADD_DATA = 1] = 'MOVING_PHOTO_ENABLED';
    e[e.DELETE_DATA = 2] = 'MOVING_PHOTO_DISABLED';
}(MovingPhotoBadgeStateType || (MovingPhotoBadgeStateType = {}));

export default { PhotoPickerComponent, PickerController, PickerOptions, DataType, BaseItemInfo, ItemInfo, PhotoBrowserInfo, AnimatorParams,
    MaxSelected, ItemType, ClickType, PickerOrientation, SelectMode, PickerColorMode, ReminderMode, MaxCountType, PhotoBrowserRange, PhotoBrowserUIElement,
    VideoPlayerState, SaveMode, SingleLineConfig, ItemDisplayRatio, BadgeOptionType, BadgeType, BadgeConfig, UpdatablePickerConfigs, MovingPhotoBadgeStateType, PickerError };

