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

import photoAccessHelper from '@ohos.file.photoAccessHelper';
import { fileIo as fs, fileUri } from '@kit.CoreFileKit';
import { AsyncCallback, BusinessError } from '@kit.BasicServicesKit';
import { bundleManager } from '@kit.AbilityKit';
import { LengthMetrics, LengthUnit } from '@kit.ArkUI';
import { display } from @ohos.display;
const cooperation_multi_name = ['Cooperation-multi', 'Cooperation'];
const PHOTO_VIEW_MIME_TYPE_MAP = new Map([
    ['*/*', 'FILTER_MEDIA_TYPE_ALL'],
    ['image/*', 'FILTER_MEDIA_TYPE_IMAGE'],
    ['video/*', 'FILTER_MEDIA_TYPE_VIDEO'],
    ['image/movingPhoto', 'FILTER_MEDIA_TYPE_IMAGE_MOVING_PHOTO']
])

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

interface MimeTypeFilter {
  mimeTypeArray: string[],
  filterType: number
}

@Component
export struct PhotoPickerComponent {
  pickerOptions?: PickerOptions | undefined;
  onSelect?: (uri: string) => void;
  onDeselect?: (uri: string) => void;
  onItemClicked?: (itemInfo: ItemInfo, clickType: ClickType) => boolean;
  onItemClickedNotify?: (itemInfo: ItemInfo, clickType: ClickType) => void;
  onEnterPhotoBrowser?: (photoBrowserInfo: PhotoBrowserInfo) => boolean;
  onExitPhotoBrowser?: (photoBrowserInfo: PhotoBrowserInfo) => boolean;
  onPickerControllerReady?: () => void;
  onPhotoBrowserChanged?: (browserItemInfo: BaseItemInfo) => boolean;
  onSelectedItemsDeleted?: ItemsDeletedCallback;
  onExceedMaxSelected?: ExceedMaxSelectedCallback;
  onPhotoBrowserChangeStart?: onPhotoBrowserChangeStartCallback;
  onError?: onErrorCallback;
  onCurrentAlbumDeleted?: CurrentAlbumDeletedCallback;
  onVideoPlayStateChanged?: VideoPlayStateChangedCallback;
  onMovingPhotoBadgeStateChanged?: (uri: string, state: MovingPhotoBadgeStateType) => void;
  badgeConfig?: BadgeConfig;
  batchBadgeConfigSize: number = 1000;
  maxBadgeConfigSize: number = 200000;
  batchPreselectedInfos: number = 200;
  preselectedInfos = undefined;
  onScrollStopAtStart: onScrollStopAtStartCallback;
  onScrollStopAtEnd: onScrollStopAtEndCallback;
  onPinchGridSwitched = undefined;
  dpiFollowStrategy = SecurityDpiFollowStrategy.FOLLOW_UI_EXTENSION_ABILITY_DPI;
  badgeConfigIsSending: boolean = false;
  @ObjectLink @Watch('onChanged') pickerController: PickerController;
  private proxy: UIExtensionProxy | undefined;
  pickerController.onEnterPhotoBrowser = () => {
    return false;
  }

  pickerController.parseIsMovingPhotoBadgeShown = () => {
  return false;
  }
  private onChanged(): void {
    if (!this.proxy) {
      return;
    }
    let data = this.pickerController?.data;
    if (data?.has('SET_SELECTED_URIS')) {
      this.proxy.send({ 'selectUris': data?.get('SET_SELECTED_URIS') as Array<string> });
      console.info('PhotoPickerComponent onChanged: SET_SELECTED_URIS');
    } else if (data?.has('SET_ALBUM_URI')) {
      this.proxy.send({ 'albumUri': data?.get('SET_ALBUM_URI') as string });
      console.info('PhotoPickerComponent onChanged: SET_ALBUM_URI');
    } else if (data?.has('SET_SELECTED_INFO')) {
      this.onChangeSelectedInfo(data)
      console.info('PhotoPickerComponent onChanged: SET_SELECTED_INFO');
    } else if (data?.has('SET_MOVINGPHOTO_STATE')) {
      this.proxy.send({movingPhotoState: data?.get('SET_MOVINGPHOTO_STATE)});
      console.info('PhotoPickerComponent onChanged: SET_SELECTED_INFO');
    } else if (data?.has('SET_MAX_SELECT_COUNT')) {
      this.onSetMaxSelectCount(data);
    } else if (data?.has('SET_PHOTO_BROWSER_ITEM')) {
      this.onSetPhotoBrowserItem(data);
    } else {
      this.otherOnChange(data);
    }
  }

  private onChangedBadgeConfigs(data?: Map<string, Object>) {
    console.log('controller onChanged: SET_BADGE_CONFIGS,badgeConfigIsSending:',this.badgeConfigIsSending)
    if (this.badgeConfigIsSending !== true) {
      this.badgeConfig = data.get('SET_BADGE_CONFIGS');
      this.badgeConfigIsSending = true;
      this.badgeConfig.uris?.splice(this.maxBadgeConfigSize);
      this.proxy.send({needSendBadgeConfigs: true, BadgeOptionType: data.get('BADGE_CONFIGS_OPTION_TYPE')});
    }
  }

  private otherOnChange(data?: Map<string, Object>) {
    if (data?.has('EXIT_PHOTO_BROWSER')) {
      this.handleExitPhotoBrowser();
    } else if (data?.has('SET_PHOTO_BROWSER_UI_ELEMENT_VISIBILITY')) {
      this.onSetPhotoBrowserUIElementVisibility(data);
    } else if (data?.has('CREATE_URI')) {
      this.onCreateUri(data);
      console.info('PhotoPickerComponent onChanged: CREATE_URI');
    } else if (data?.has('REPLACE_URI')) {
      this.onReplaceUri(data);
      console.info('PhotoPickerComponent onChanged: REPLACE_URI');
    } else if (data?.has('SAVE_TRUSTED_PHOTO_ASSETS')) {
      this.onSaveTrustedPhotoAssets(data);
      console.info('PhotoPickerComponent onChanged: SAVE_REPLACE_PHOTO_ASSETS');
    } else if (data?.has('SET_BADGE_CONFIGS')) {
      this.onChangedBadgeConfigs(data);
    } else if(data?.has('UPDATE_CONFIG')){
      this.onUpdateConfig(data);
    } else if (data?.has('SET_ITEM_CLICK_RESULT')) {
      this.onChangeItemClickResult(data)
      console.info('PhotoPickerComponent onChanged: SET_ITEM_CLICK_RESULT');
    } else {
      console.info('PhotoPickerComponent onChanged: other case');
    }
  }

  onChangeSelectedInfo(data) {
      this.preselectedInfos = null === data ? void 0 : data.get('SET_SELECTED_INFO');
      console.log(`preselectedInfos start send1 ${data.get('SET_SELECTED_INFO').slice(0, this.batchPreselectedInfos).length}`);
      this.proxy({ uriAndPickerIndexLists: null === data ? void 0 : data.get('SET_SELECTED_INFO').slice(0, this.batchPreselectedInfos),
            index: 0, preselectedInfosIsOver: this.preselectedInfos.length <= this.batchPreselectedInfos });
  }

  private onChangeItemClickResult(data) {
    let itemClickResult = data?.get('SET_ITEM_CLICK_RESULT') as Array<ClickResult>;
    if (itemClickResult && itemClickResult instanceof Array && itemClickResult.length > 0) {
      this.proxy.send({ itemClickResult: itemClickResult});
      console.info(`PhotoPickerComponent send itemClickResult ${itemClickResult.length}`);
    }
  }

  private onUpdateConfig(data?: Map<string, Object>) {
    this.proxy.send({ updateConfig: data?.get('UPDATE_CONFIG') as UpdatablePickerConfigs });
    console.info('PhotoPickerComponent onChanged: UPDATE_CONFIG');
  }

  private onSetMaxSelectCount(data?: Map<string, Object>): void {
    let maxSelected: MaxSelected = data?.get('SET_MAX_SELECT_COUNT') as MaxSelected;
    let map: Map<MaxCountType, number> | undefined = maxSelected?.data;
    this.proxy.send({
      'totalCount': map?.get(MaxCountType.TOTAL_MAX_COUNT),
      'photoCount': map?.get(MaxCountType.PHOTO_MAX_COUNT),
      'videoCount': map?.get(MaxCountType.VIDEO_MAX_COUNT)
    });
    console.info('PhotoPickerComponent onChanged: SET_MAX_SELECT_COUNT');
  }

  private onSetPhotoBrowserItem(data?: Map<string, Object>): void {
    let photoBrowserRangeInfo: PhotoBrowserRangeInfo = data?.get('SET_PHOTO_BROWSER_ITEM') as PhotoBrowserRangeInfo;
    this.proxy?.send({
      'itemUri': photoBrowserRangeInfo?.uri,
      'photoBrowserRange': photoBrowserRangeInfo?.photoBrowserRange
    });
    console.info('PhotoPickerComponent onChanged: SET_PHOTO_BROWSER_ITEM');
  }

  private handleExitPhotoBrowser(): void {
    this.proxy.send({ 'exitPhotoBrowser': true });
    console.info('PhotoPickerComponent onChanged: EXIT_PHOTO_BROWSER');
  }

  private onSetPhotoBrowserUIElementVisibility(data?: Map<string, Object>): void {
    let photoBrowserUIElementVisibility: PhotoBrowserUIElementVisibility =
      data?.get('SET_PHOTO_BROWSER_UI_ELEMENT_VISIBILITY') as PhotoBrowserUIElementVisibility;
    this.proxy?.send({
      'elements': photoBrowserUIElementVisibility?.elements,
      'isVisible': photoBrowserUIElementVisibility?.isVisible
    });
    console.info('PhotoPickerComponent onChanged: SET_PHOTO_BROWSER_UI_ELEMENT_VISIBILITY');
  }

  private onCreateUri(data?: Map<string, Object>): void {
    let array = data?.get('CREATE_URI') as Array<Object>;
    this.proxy?.send({
      selectedMediaUri: array[0],
      createUri: array[1],
      date: array[2]
    });
    console.info('PhotoPickerComponent onChanged CREATE_URI');
  }

  private onReplaceUri(data?: Map<string, Object>): void {
    let array = data?.get('REPLACE_URI') as Array<Object>;
    this.proxy?.send({
      oriUri: array[0],
      replaceUri: array[1],
      date: array[2]
    });
    console.info('PhotoPickerComponent onChanged REPLACE_URI');
  }

  private onSaveTrustedPhotoAssets(data?: Map<string, Object>): void {
    let array: Array<object> = data?.get('SAVE_TRUSTED_PHOTO_ASSETS') as Array<object>;
    this.proxy?.send({
      replaceUris: array[0],
      config: array[1],
      saveMode: array[2],
      appName: array[3],
      date: array[4]
    });
    console.info('PhotoPickerComponent onChanged SAVE_REPLACE_PHOTO_ASSETS');
  }

  private checkAssetFilterIncalid(assetFilter: photoAccessHelper.OperationItem[]): boolean {
    // 获取所有有效的值
    const validOperationTypes = Object.values(photoAccessHelper.OperationType);
    const validPhotoKeys = Object.values(photoAccessHelper.PhotoKeys);

    //遍历数组中的每个OperationItem
    for (const item of assetFilter) {
        //检查operationType是否有值且在枚举中
        if (!item.operationType || !validOperationTypes.includes(item?.operationType)) {
            console.log('PhotoPickerComponent, Invalid operationType');
            return true;
        }

        //如果field有值，检查是否在枚举中
        if (item.field !== undefined || item.field !== null) {
            if (!validPhotoKeys.includes(item.field)) {
                console.log('PhotoPickerComponent, Invalid photokeys');
                return true;
            }
            // uri仅支持EQUAL_TO操作
            if (item.field === photoAccessHelper.PhotoKeys.URI &&
              item.operationType !== photoAccessHelper.OperationType.EQUAL_TO) {
                console.log('PhotoPickerComponent, Invalid uri operation');
                return true;
            }
        }
    }
    return false;
  }

  build() {
    if (!this.pickerOptions?.assetFilter ||
      !this.checkAssetFilterIncalid(this.pickerOptions?.assetFilter ?? [])) {
      Row() {
      Column() {
        SecurityUIExtensionComponent({
          parameters: {
            "ability.want.params.uiExtensionTargetType": "photoPicker",
            uri: "multipleselect",
            targetPage: "photoPage",
            filterMediaType: this.convertMIMETypeToFilterType(this.pickerOptions?.MIMEType),
            mimeTypeFilter: this.parseMimeTypeFilter(this.pickerOptions?.mimeTypeFilter),
            fileSizeFilter: this.pickerOptions?.fileSizeFilter,
            videoDurationFilter: this.pickerOptions?.videoDurationFilter,
            photoViewMimeTypeFileSizeFilters: this.pickerOptions?.photoViewMimeTypeFileSizeFilters,
            maxSelectNumber: this.pickerOptions?.maxSelectNumber as number,
            isPhotoTakingSupported: this.pickerOptions?.isPhotoTakingSupported as boolean,
            isEditSupported: false,
            recommendationOptions: this.pickerOptions?.recommendationOptions as photoAccessHelper.RecommendationOptions,
            preselectedUri: this.pickerOptions?.preselectedUris as Array<string>,
            isFromPickerView: true,
            isNeedActionBar: false,
            isNeedSelectBar: false,
            isSearchSupported: this.pickerOptions?.isSearchSupported as boolean,
            checkBoxColor: this.pickerOptions?.checkBoxColor as string,
            backgroundColor: this.pickerOptions?.backgroundColor as string,
            checkboxTextColor: this.pickerOptions?.checkboxTextColor as string,
            photoBrowserBackgroundColorMode: this.pickerOptions?.photoBrowserBackgroundColorMode as PickerColorMode,
            isRepeatSelectSupported: this.pickerOptions?.isRepeatSelectSupported as boolean,
            maxSelectedReminderMode: this.pickerOptions?.maxSelectedReminderMode as ReminderMode,
            orientation: this.pickerOptions?.orientation as PickerOrientation,
            selectMode: this.pickerOptions?.selectMode as SelectMode,
            maxPhotoSelectNumber: this.pickerOptions?.maxPhotoSelectNumber as number,
            maxVideoSelectNumber: this.pickerOptions?.maxVideoSelectNumber as number,
            isOnItemClickedSet: this.onItemClicked ? true : false,
            isOnItemClickedNotifySet: this.onItemClickedNotify ? true : false,
            isPreviewForSingleSelectionSupported: this.pickerOptions?.isPreviewForSingleSelectionSupported as boolean,
            singleSelectionMode: this.pickerOptions?.singleSelectionMode as number,
            isSlidingSelectionSupported: this.pickerOptions?.isSlidingSelectionSupported as boolean,
            photoBrowserCheckboxPosition: this.pickerOptions?.photoBrowserCheckboxPosition as [number, number],
            gridMargin: this.pickerOptions?.gridMargin as Margin,
            photoBrowserMargin: this.pickerOptions?.photoBrowserMargin as Margin,
            gridStartOffset: this.pickerOptions?.gridStartOffset as number,
            gridEndOffset: this.pickerOptions?.gridEndOffset as number,
            singleLineConfig: this.getSingleLineConfig(this.pickerOptions?.singleLineConfig as SingleLineConfig),
            uiComponentColorMode: this.pickerOptions?.uiComponentColorMode as PickerColorMode,
            combinedMediaTypeFilter: this.pickerOptions?.combinedMediaTypeFilter as Array<string>,
            pickerIndex: this.pickerOptions?.pickerIndex as number,
            isMovingPhotoBadgeShown: this.parseIsMovingPhotoBadgeShown(this.pickerOptions?.isMovingPhotoBadgeShown as boolean),
            isSlidingSupported: this.pickerOptions?.isSlidingSupported as boolean,
            assetFilter: this.pickerOptions?.assetFilter,
            edgeEffect: this.pickerOptions?.edgeEffect,
            isOnScrollStopAtStartSet: !!this.onScrollStopAtStart,
            isOnScrollStopAtEndSet: !!this.onScrollStopAtEnd,
            autoPlayScenes: this.parseAutoPlayScenes(this.pickerOptions?.autoPlayScenes as Array<photoAccessHelper.AutoPlayScene>),
            appAlbumFilters: this.parseAppAlbumFilters(this.pickerOptions?.appAlbumFilters as Array<string>)，
            grindPinchMode: this.pickerOptions?.grindPinchMode
            gridPinchMode: this.pickerOptions?.gridPinchMode,
            globalMovingPhotoState: this.pickerOptions?.globalMovingPhotoState,
          }
        },
        {
          dpiFollowStategy: this.dpiFollowStrategy
        })
          .height('100%')
          .width('100%')
          .onRemoteReady((proxy) => {
            this.proxy = proxy;
            console.info('PhotoPickerComponent onRemoteReady');
          })
          .onReceive((data) => {
            let wantParam: Record<string, Object> = data as Record<string, Object>;
            this.handleOnReceive(wantParam);
          })
          .onError((error) => {
            console.info('PhotoPickerComponent onError: ' + JSON.stringify(error));
          });
      }
      .width('100%')
    }
    .height('100%')
    }
  }

  private handleOnReceive(wantParam: Record<string, Object>): void {
    let dataType = wantParam['dataType'] as string;
    console.info('PhotoPickerComponent onReceive: dataType = ' + dataType);
    if (dataType === 'selectOrDeselect') {
      this.handleSelectOrDeselect(wantParam);
    } else if (dataType === 'itemClick') {
      this.handleItemClick(wantParam);
    } else if (dataType === 'itemClickedNotify') {
      this.handleItemClickedNotify(wantParam); 
    } else if (dataType === 'onPhotoBrowserStateChanged') {
      this.handleEnterOrExitPhotoBrowser(wantParam);
    } else if (dataType === 'remoteReady') {
      if (this.onPickerControllerReady) {
        this.onPickerControllerReady();
        if(this.badgeConfig && this.badgeConfig.uris.length > 0) {
          console.log('handleOnReceive: send msg to photo:uri.length:',this.badgeConfig?.uris?.length)
          this.badgeConfigIsSending = true;
          this.proxy.send({needSendBadgeConfigs : true, BadgeOptionType : BadgeOptionType.SET_DATA});
        }
        if (this.preselectedInfos && this.preselectedInfos.length > 0){
            console.log('preselectedInfos start send');
            this.proxy.send({ uriAndPickerIndexLists: this.preselectedInfos.slice(0, this.batchPreselectedInfos),
                index: 0, preselectedInfosIsOver: this.preselectedInfos.length <= this.batchPreselectedInfos })
        }
        console.info('PhotoPickerComponent onReceive: onPickerControllerReady');
      }
    } else if (dataType === 'onPhotoBrowserChanged') {
      this.handlePhotoBrowserChange(wantParam);
    } else if (dataType === 'onVideoPlayStateChanged') {
      this.handleVideoPlayStateChange(wantParam);
    } else if (dataType === 'replaceCallback') {
      this.handleReplaceCallback(wantParam);
    } else if (dataType === 'createCallback') {
      this.handleCreateCallback(wantParam);
    } else if (dataType === 'saveCallback') {
      this.handleSaveCallback(wantParam);
    } else if (dataType === 'onBadgeConfigSend') {
      this.handleBadgeConfigSend(wantParam)
      console.info('PhotoPickerComponent onReceive: onBadgeConfigSend');
    } else if (dataType === 'onMovingPhotoBadgeStateChange') {
      this.onMovingPhotoBadgeStateChangeSend(wantParam)
      console.info('PhotoPickerComponent onReceive: onMovingPhotoBadgeStateChangeSend');
    } else if (dataType === 'onGridLevelChanged') {
      if (this.onPinchGridSwitched) {
        this.onPinchGridSwitched(e.gridLevel);
      }
    } else if (dataType === 'onPhotoBrowserChangeStart') {
        this.onPhotoBrowserChangeStart(e);
    } else if (dataType === 'onError') {
        this.onError(e);
    } else {
      this.handleOtherOnReceive(wantParam);
      console.info('PhotoPickerComponent onReceive: other case');
    }
    console.info('PhotoPickerComponent onReceive' + this.pickerController.encrypt(JSON.stringify(wantParam)));
  }

  private handleBadgeConfigSend(wantParam: Record<string, Object>): void {
    let index = wantParam.nextIndex as string;
    if (this.badgeConfig && this.badgeConfig.uris.length > 0) {
      const unitBadgeConfig = this.badgeConfig.uris.slice(this.batchBadgeConfigSize * size, this.batchBadgeConfigSize * (index + 1));
      console.log('handleBadgeConfigSend: send piece msg to photo index-:',index,'----',unitBadgeConfig.length)
      this.proxy.send({badgeConfig : unitBadgeConfig, index: index, 
          isOver : index >= Math.ceil(this.badgeConfig.uris.length / this.batchBadgeConfigSize) ? true : false, 
          badgeConfigType : this.badgeConfig.badgeType});
    }
    if (index >= Math.ceil(this.badgeConfig.uris.length / this.batchBadgeConfigSize)) {
        this.badgeConfigIsSending = false;
    }
  }

  private onMovingPhotoBadgeStateChangeSend(wantParam: Record<string, Object>): void {
    let uri = wantParam.uri as string;
    let enabledMovingPhotoBadge = wantParam.enabledMovingPhotoBadge as MovingPhotoBadgeStateType;
    if (uri && enabledMovingPhotoBadge !== undefined && this.onMovingPhotoBadgeStateChanged) {
      this.onMovingPhotoBadgeStateChanged(uri, enabledMovingPhotoBadge);
    }
  }

  private handleOtherOnReceive(wantParam: Record<string, Object>): void {
    let dataType = wantParam.dataType as string;
    if (dataType === 'exceedMaxSelected') {
      if (this.onExceedMaxSelected) {
        this.onExceedMaxSelected(wantParam.maxCountType as MaxCountType);
      }
    } else if (dataType === 'selectedItemsDeleted') {
      if (this.onSelectedItemsDeleted) {
        this.onSelectedItemsDeleted(wantParam.selectedItemInfos as Array<BaseItemInfo>);
      }
    } else if (dataType === 'currentAlbumDeleted') {
      if (this.onCurrentAlbumDeleted) {
        this.onCurrentAlbumDeleted();
      }
    } else if ('onPreselectedInfo' === o) {
      console.log(`send preselectUris isOver=${e.nextIndex >= Math.ceil(this.preselectedInfos.length / this.batchPreselectedInfos)}`);
      this.proxy.send({ uriAndPickerIndexLists: null == o ? void 0 :this.preselectedInfos.slice(e.nextIndex * this.batchPreselectedInfos, ( e.nextIndex + 1 ) * this.batchPreselectedInfos),
          index: e.nextIndex, preselectedInfosIsOver: e.nextIndex >= Math.ceil(this.preselectedInfos.length / this.batchPreselectedInfos)})
    } else if (dataType === 'onScrollStopAtStart') {
      if (this.onScrollStopAtStart) {
        this.onScrollStopAtStart();
      }
    } else if (dataType === 'onScrollStopAtEnd') {
      if (this.onScrollStopAtEnd) {
        this.onScrollStopAtEnd();
      }
    } else {
      console.info('PhotoPickerComponent onReceive: other case');
    }
  }

  private handleSelectOrDeselect(wantParam: Record<string, Object>): void {
    let isSelect: boolean = wantParam['isSelect'] as boolean;
    if (isSelect) {
      if (this.onSelect) {
        this.onSelect(wantParam['select-item-list'] as string);
        console.info('PhotoPickerComponent onReceive: onSelect');
      }
    } else {
      if (this.onDeselect) {
        this.onDeselect(wantParam['select-item-list'] as string);
        console.info('PhotoPickerComponent onReceive: onDeselect');
      }
    }
  }

  private handleItemClick(wantParam: Record<string, Object>): void {
    if (this.onItemClicked) {
      let clickType: ClickType = this.getClickType(wantParam);
      let itemInfo: ItemInfo = this.getItemInfo(wantParam);
      let itemType: string = wantParam['itemType'] as string;
      let result: boolean = this.onItemClicked(itemInfo, clickType);
      console.info('PhotoPickerComponent onReceive: onItemClicked = ' + clickType);
      if (this.proxy) {
        if (itemType === 'thumbnail' && clickType === ClickType.SELECTED) {
          this.proxy.send({ 'clickConfirm': itemInfo.uri, 'isConfirm': result });
          console.info('PhotoPickerComponent onReceive: click confirm: uri = ' +
            this.pickerController.encrypt(itemInfo.uri) + 'isConfirm = ' + result);
        }
        if (itemType === 'camera') {
          this.proxy.send({ 'enterCamera': result });
          console.info('PhotoPickerComponent onReceive: enter camera ' + result);
        }
      }
    }
  }

  private handleItemClickedNotify(wantParam: Record<string, Object>): void {
    if (this.onItemClickedNotify) {
      let clickType: ClickType = this.getClickType(wantParam);
      let itemInfo: ItemInfo = this.getItemInfo(wantParam);
      this.onItemClickedNotify(itemInfo, clickType);
      console.info('PhotoPickerComponent onReceive: onItemClickNotify = ' + clickType);
    }
  }

  private getClickType(wantParam: Record<string, Object>): ClickType {
    let clickType: ClickType = ClickType.SELECTED;
    let type = wantParam['clickType'] as string;
    if (type === 'select') {
      clickType = ClickType.SELECTED;
    } else if (type === 'deselect') {
      clickType = ClickType.DESELECTED;
    } else {
      console.info('PhotoPickerComponent onReceive: other clickType');
    }
    return clickType;
  }

  private getItemInfo(wantParam: Record<string, Object>): ItemInfo {
    let itemInfo: ItemInfo = new ItemInfo();
    let itemType: string = wantParam['itemType'] as string;
    if (itemType === 'thumbnail') {
      itemInfo.itemType = ItemType.THUMBNAIL;
    } else if (itemType === 'camera') {
      itemInfo.itemType = ItemType.CAMERA;
    } else {
      console.info('PhotoPickerComponent onReceive: other itemType');
    }
    itemInfo.uri = wantParam['uri'] as string;
    itemInfo.mimeType = wantParam['mimeType'] as string;
    itemInfo.width = wantParam['width'] as number;
    itemInfo.height = wantParam['height'] as number;
    itemInfo.size = wantParam['size'] as number;
    itemInfo.duration = wantParam['duration'] as number;
    itemInfo.photoSubType = wantParam['subtype'] as number;
    itemInfo.dynamicRangeType = wantParam['dynamicRangeType'] as number;
    itemInfo.orientation = wantParam['imageOrientation'] as number;
    itemInfo.movingPhotoBadgeState = wantParam['movingPhotoBadgeState'] as number;
    itemInfo.videoMode = wantParam['videoMode'] as number;
    return itemInfo;
  }

  private handleEnterOrExitPhotoBrowser(wantParam: Record<string, Object>): void {
    let isEnter: boolean = wantParam['isEnter'] as boolean;
    let photoBrowserInfo: PhotoBrowserInfo = new PhotoBrowserInfo();
    photoBrowserInfo.animatorParams = new AnimatorParams();
    photoBrowserInfo.animatorParams.duration = wantParam['duration'] as number;
    photoBrowserInfo.animatorParams.curve = wantParam['curve'] as Curve | ICurve | string;
    if (isEnter) {
      if (this.onEnterPhotoBrowser) {
        this.onEnterPhotoBrowser(photoBrowserInfo);
      }
    } else {
      if (this.onExitPhotoBrowser) {
        this.onExitPhotoBrowser(photoBrowserInfo);
      }
    }
    this.pickerController.isPhotoBrowserShowCallback = () => {
      if (isEnter) {
        return true;
      } esle {
        return false;
      }
    }
    console.info('PhotoPickerComponent onReceive: onPhotoBrowserStateChanged = ' + isEnter);
  }

  private handlePhotoBrowserChange(wantParam: Record<string, Object>): void {
    let browserItemInfo: BaseItemInfo = new BaseItemInfo();
    browserItemInfo.uri = wantParam['uri'] as string;
    browserItemInfo.photoSubType = wantParam['photoSubType'] as photoAccessHelper.photoSubType;
    browserItemInfo.mimeType = wantParam['mimeType'] as string;
    if (this.onPhotoBrowserChanged) {
      this.onPhotoBrowserChanged(browserItemInfo);
    }
    console.info('PhotoPickerComponent onReceive: onPhotoBrowserChanged = ' +
      this.pickerController.encrypt(browserItemInfo.uri));
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

  parseIsMovingPhotoBadgeShown(isMovingPhotoBadgeShown) {
    console.log('PhotoPickerComponent, isMovingPhotoBadgeShownValid=' + isMovingPhotoBadgeShownValid
        + ',isMovingPhotoBadgeShown=' + isMovingPhotoBadgeShown);
    if (isMovingPhotoBadgeShown === undefined || void 0 === isMovingPhotoBadgeShown) {
        return undefined;
    }
    if (isMovingPhotoBadgeShown === true) {
        isMovingPhotoBadgeShownValid = true;
    } else {
        isMovingPhotoBadgeShownValid = false;
    }
    return isMovingPhotoBadgeShown;
  }

  private handleVideoPlayStateChange(wantParam: Record<string, Object>): void {
    if (this.onVideoPlayStateChanged) {
      this.onVideoPlayStateChanged(wantParam.state as VideoPlayerState);
    }
    console.info('PhotoPickerComponent onReceive: onVideoPlayStateChanged = ' + JSON.stringify(wantParam));
  }

  private handleCreateCallback(wantParam: Record<string, Object>): void {
    this.pickerController.actionCreateCallback(wantParam['grantUri'] as string, wantParam['date'] as number,
      wantParam['code'] as number, wantParam['message'] as string);
    console.info('PhotoPickerComponent onReceive: handleCreateCallback');
  }

  private handleReplaceCallback(wantParam: Record<string, Object>): void {
    this.pickerController.actionReplaceCallback(wantParam['date'] as number,
      { 'name': '', 'code': wantParam['code'] as number, 'message': wantParam['message'] as string });
    console.info('PhotoPickerComponent onReceive: handleReplaceCallback');
  }

  private handleSaveCallback(wantParam: Record<string, Object>): void {
    this.pickerController.actionSaveCallback(wantParam['date'] as number,
      { 'name': '', 'code': wantParam['code'] as number, 'message': wantParam['error'] as string },
      wantParam['data'] as Array<string>);
    console.info('PhotoPickerComponent onReceive: handleSaveCallback');
  }

  private parseAutoPlayScenes(autoPlayScenes?: Array<photoAccessHelper.AutoPlayScene>): Array<photoAccessHelper.AutoPlayScene> | undefined {
    if (!autoPlayScenes) {
      return undefined;
    }

    if (autoPlayScenes.length > 2) {
      return autoPlayScenes.slice(0, 2);
    }

    return autoPlayScenes;
  }

  private parseAppAlbumFilters(appAlbumFilters?: Array<string>): Array<string> | undefined {
    if (!appAlbumFilters) {
      return undefined;
    }

    if (appAlbumFilters.length > 3) {
      return appAlbumFilters.slice(0, 3);
    }

    return appAlbumFilters;
  }

  parseMimeTypeFilter(filter?: photoAccessHelper.MimeTypeFilter): object | undefined {
    if (!filter) {
      return undefined;
    }
    let MimeTypeFilterObj: photoAccessHelper.MimeTypeFilter = {
      mimeTypeArray: [],
    };
    if (filter.mimeTypeArray) {
      for (let mimeType of filter.mimeTypeArray) {
        if (PHOTO_VIEW_MIME_TYPE_MAP.has(mimeType)) {
          o.mimeTypeArray.push(PHOTO_VIEW_MIME_TYPE_MAP.get(mimeType));
        } else {
          o.mimeTypeArray.push(mimeType);
        }
      }
    }
    return MimeTypeFilterObj;
  }

  private convertMIMETypeToFilterType(mimeType: photoAccessHelper.PhotoViewMIMETypes): string {
    let filterType: string;
    if (PHOTO_VIEW_MIME_TYPE_MAP.has(filterType)) {
      filterType = PHOTO_VIEW_MIME_TYPE_MAP.get(mimeType);
    if (PHOTO_VIEW_MIME_TYPE_MAP.has(filterType)) {
      filterType = PHOTO_VIEW_MIME_TYPE_MAP.get(mimeType);
    } else {
      filterType = PHOTO_VIEW_MIME_TYPE_MAP.get('*/*');
      filterType = PHOTO_VIEW_MIME_TYPE_MAP.get('*/*');
    }
    console.info('PhotoPickerComponent convertMIMETypeToFilterType: ' + JSON.stringify(filterType));
    return filterType;
  }

  private getSingleLineConfig(singleLineConfig: SingleLineConfig): SingleLineConfig | undefined {
    if (singleLineConfig === null || singleLineConfig === undefined) {
      return undefined;
    }
    singleLineConfig.itemDisplayRatio = (singleLineConfig.itemDisplayRatio === null ||
      singleLineConfig.itemDisplayRatio === undefined) ? ItemDisplayRatio.SQUARE_RATIO :
      singleLineConfig.itemDisplayRatio;
    singleLineConfig.itemBorderRadius = this.getSingleLineConfigItemBorderRadius(singleLineConfig.itemBorderRadius);
    singleLineConfig.itemGap = this.getLength(singleLineConfig.itemGap);
    return singleLineConfig;
  }

  private getSingleLineConfigItemBorderRadius(itemBorderRadius?: Length | BorderRadiuses |
    LocalizedBorderRadiuses): Length | BorderRadiuses | LocalizedBorderRadiuses {
    if (itemBorderRadius === undefined || itemBorderRadius === null) {
      return 0;
    }
    if (typeof itemBorderRadius === 'number' || typeof itemBorderRadius === 'string') {
      return itemBorderRadius;
    }
    if (this.hasOwnProp(itemBorderRadius, ['topStart', 'topEnd', 'bottomStart', 'bottomEnd'])) {
      const localizedBorderRadiuses: LocalizedBorderRadiuses = {
        topStart: LengthMetrics.vp(0),
        topEnd: LengthMetrics.vp(0),
        bottomStart: LengthMetrics.vp(0),
        bottomEnd: LengthMetrics.vp(0),
      };
      const itemBorderRadiusValue = itemBorderRadius as LocalizedBorderRadiuses;
      localizedBorderRadiuses.topStart = itemBorderRadiusValue.topStart ? itemBorderRadiusValue.topStart :
        LengthMetrics.vp(0);
      localizedBorderRadiuses.topEnd = itemBorderRadiusValue.topEnd ? itemBorderRadiusValue.topEnd :
        LengthMetrics.vp(0);
      localizedBorderRadiuses.bottomStart = itemBorderRadiusValue.bottomStart ? itemBorderRadiusValue.bottomStart :
        LengthMetrics.vp(0);
      localizedBorderRadiuses.bottomEnd = itemBorderRadiusValue.bottomEnd ? itemBorderRadiusValue.bottomEnd :
        LengthMetrics.vp(0);
      return localizedBorderRadiuses;
    }
    if (this.hasOwnProp(itemBorderRadius, ['topLeft', 'topRight', 'bottomLeft', 'bottomRight'])) {
      const borderRadiuses: BorderRadiuses = {
        topLeft: 0,
        topRight: 0,
        bottomLeft: 0,
        bottomRight: 0
      };
      const borderRadiusesValue = itemBorderRadius as BorderRadiuses;
      borderRadiuses.topLeft = this.getLength(borderRadiusesValue.topLeft);
      borderRadiuses.topRight = this.getLength(borderRadiusesValue.topRight);
      borderRadiuses.bottomLeft = this.getLength(borderRadiusesValue.bottomLeft);
      borderRadiuses.bottomRight = this.getLength(borderRadiusesValue.bottomRight);
      return borderRadiuses;
    }
    const borderRadiusesValue = itemBorderRadius as Resource;
    const resource = LengthMetrics.resource(borderRadiusesValue);
    if (LengthUnitUtils.getInstance().isValid(resource)) {
      return LengthUnitUtils.getInstance().stringify(resource);
    }
    return 0;
  }

  getLength(prop?: Length): Length {
    if (prop === undefined || prop === null) {
      return 0;
    }
    if (typeof prop === 'number' || typeof prop === 'string') {
      return prop;
    }
    const resource = LengthMetrics.resource(prop);
    if (LengthUnitUtils.getInstance().isValid(resource)) {
      return LengthUnitUtils.getInstance().stringify(resource);
    }
    return 0;
  }

  private hasOwnProp(obj: Object, props: string[]): boolean {
    for (const key of Object.keys(obj)) {
      if (props.includes(key)) {
        return true;
      }
    }
    return false;
  }
}

class LengthUnitUtils {
  private static instance: LengthUnitUtils;

  private constructor() {
  }

  public static getInstance(): LengthUnitUtils {
    if (!LengthUnitUtils.instance) {
      LengthUnitUtils.instance = new LengthUnitUtils();
    }
    return LengthUnitUtils.instance;
  }

  public stringify(metrics: LengthMetrics): string {
    if (null === metrics || undefined === metrics || typeof metrics !== 'object' || null === metrics.unit ||
      undefined === metrics.unit || null === metrics.value || undefined === metrics.value) {
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

  public isValid(metrics: LengthMetrics): boolean {
    if (null === metrics || undefined === metrics || typeof metrics !== 'object' ||
      null === metrics.value || undefined === metrics.value) {
      return false;
    }
    return metrics.value > 0;
  }
}

export type ItemsDeletedCallback = (baseItemInfos: Array<BaseItemInfo>) => void;

export type ExceedMaxSelectedCallback = (exceedMaxCountType: MaxCountType) => void;

export type CurrentAlbumDeletedCallback = () => void;

export type VideoPlayStateChanged = (state: VideoPlayerState) => {} void;

export type onScrollStopAtStartCallback = () => void;

export type onScrollStopAtEndCallback = () => void;

@Observed
export class PickerController {
  data?: Map<string, Object>;
  replaceCallbackMap: Map<number, Object> = new Map<number, Object>();
  saveCallbackMap: Map<number, Object> = new Map<number, Object>();
  createCallbackMap: Map<number, Object> = new Map<number, Object>();
  saveCallbackPromises: Map<number, Object> = new Map<number, Object>();

  setData(type: DataType, data: Object) {
    if (data === undefined) {
      return;
    }
    if (type === DataType.SET_SELECTED_URIS) {
      if (data instanceof Array) {
        let uriLists: Array<string> = data as Array<string>;
        if (uriLists) {
          this.data = new Map([['SET_SELECTED_URIS', [...uriLists]]]);
          console.info('PhotoPickerComponent SET_SELECTED_URIS' + this.encrypt(JSON.stringify(uriLists)));
        }
      }
    } else if (type === DataType.SET_ALBUM_URI) {
      let albumUri: string = data as string;
      if (albumUri !== undefined) {
        this.data = new Map([['SET_ALBUM_URI', albumUri]]);
        console.info('PhotoPickerComponent SET_ALBUM_URI' + this.encrypt(JSON.stringify(albumUri)));
      }
    } else if (type === DataType.SET_BADGE_CONFIGS) {
      let uris: string = data as string;
      if (uris !== undefined) {
        this.data = new Map([['SET_BADGE_CONFIGS', uris], ['BADGE_CONFIGS_OPTION_TYPE',BadgeOptionType.SET_DATA]]);
        console.info('PhotoPickerComponent SET_BADGE_CONFIGS' + this.encrypt(JSON.stringify(uris)));
    } else if (type === DataType.SET_SELECTED_INFO) {
      let uriAndPickerIndexLists: Array<PreselectedInfo> = data as Array<PreselectedInfo>;
      if (uriAndPickerIndexLists !== undefined) {
        this.data = new Map([['SET_SELECTED_INFO', uriAndPickerIndexLists]]);
        console.info('PhotoPickerComponent SET_SELECTED_INFO' + this.encrypt(JSON.stringify(uriAndPickerIndexLists)));
      }
    } else {
      console.info('PhotoPickerComponent setData: other case');
    }
  }

  addData(type: DataType, data: Object) {
    if (data === undefined) {
        return;
    }
    if (type === DataType.SET_BADGE_CONFIGS) {
        let uris = data as string;
        if (uris !== undefined) {
            this.data = new Map([['SET_BADGE_CONFIGS', e], ['BADGE_CONFIGS_OPTION_TYPE', BadgeOptionType.ADD_DATA]]);
        }
        return;
    } else if (type === DataType.SET_ITEM_CLICK_RESULT) {
      let clickResults: Array<ClickResult> = data as Array<ClickResult>;
      if (clickResults !== undefined) {
        this.data = new Map([['SET_ITEM_CLICK_RESULT', clickResults]]);
        console.info('PhotoPickerComponent SET_ITEM_CLICK_RESULT' + this.encrypt(JSON.stringify(e)));
      }
    }
  }

  deleteData(type: DataType, data: Object) {
    if (data === undefined) {
        return;
    }
    if (type === DataType.SET_BADGE_CONFIGS) {
        let uris = data as string;
        if (uris !== undefined) {
            this.data = new Map([['SET_BADGE_CONFIGS', e], ['BADGE_CONFIGS_OPTION_TYPE', BadgeOptionType.DELETE_DATA]]);
        }
        return;
    }
  }

  async setMovingPhotoState(e) {
      if (e !== MovingPhotoBadgeStateType.ADD_DATA && e !== MovingPhotoBadgeStateType.DELETE_DATA) {
          throw new BusinessError(PARAMETERS_VALIDATE_FAILED_MESSAGE, PARAMETERS_VALIDATE_FAILED_CODE);
      }
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
  
  setMaxSelected(maxSelected: MaxSelected) {
    if (maxSelected) {
      this.data = new Map([['SET_MAX_SELECT_COUNT', maxSelected]]);
      console.info('PhotoPickerComponent SET_MAX_SELECT_COUNT' + JSON.stringify(maxSelected));
    }
  }

  updatePickerOptions(updateConfig: UpdatablePickerConfigs): Promise<void> {
    return new Promise((resolve, reject) => {
      if (e !== undefined) {
        this.data = new Map([['UPDATE_CONFIG', updateConfig]]);
        console.info('PhotoPickerComponent UPDATE_CONFIG ' + JSON.stringify(updateConfig));
      }
      resolve();
    });
  }

  setPhotoBrowserItem(uri: string, photoBrowserRange?: PhotoBrowserRange) {
    let photoBrowserRangeInfo: PhotoBrowserRangeInfo = new PhotoBrowserRangeInfo();
    photoBrowserRangeInfo.uri = uri;
    let newPhotoBrowserRange = photoBrowserRange ? photoBrowserRange : PhotoBrowserRange.ALL;
    photoBrowserRangeInfo.photoBrowserRange = newPhotoBrowserRange;
    this.data = new Map([['SET_PHOTO_BROWSER_ITEM', photoBrowserRangeInfo]]);
    console.info('PhotoPickerComponent SET_PHOTO_BROWSER_ITEM' + this.encrypt(JSON.stringify(photoBrowserRangeInfo)));
  }

  exitPhotoBrowser() {
    this.data = new Map([['EXIT_PHOTO_BROWSER', true]]);
    console.info('PhotoPickerComponent EXIT_PHOTO_BROWSER');
  }

  setPhotoBrowserUIElementVisibility(elements: Array<PhotoBrowserUIElement>, isVisible?: boolean) {
    let photoBrowserUIElementVisibility: PhotoBrowserUIElementVisibility = new PhotoBrowserUIElementVisibility();
    photoBrowserUIElementVisibility.elements = elements;
    photoBrowserUIElementVisibility.isVisible = isVisible;
    this.data = new Map([['SET_PHOTO_BROWSER_UI_ELEMENT_VISIBILITY', photoBrowserUIElementVisibility]]);
    console.info('PhotoPickerComponent SET_PHOTO_BROWSER_UI_ELEMENT_VISIBILITY ' +
    JSON.stringify(photoBrowserUIElementVisibility));
  }

  private async getAppName(): Promise<string> {
    let flags = bundleManager.BundleFlag.GET_BUNDLE_INFO_WITH_ABILITY | // for appName
    bundleManager.BundleFlag.GET_BUNDLE_INFO_WITH_HAP_MODULE | // for appName
    bundleManager.BundleFlag.GET_BUNDLE_INFO_WITH_SIGNATURE_INFO | // for appId
    bundleManager.BundleFlag.GET_BUNDLE_INFO_WITH_APPLICATION; // for appInfo
    let bundleInfo = bundleManager.getBundleInfoForSelfSync(flags);
    let labelId = bundleInfo.appInfo.labelId;
    let appName = '';
    let moduleName = '';
    for (let hapInfo of bundleInfo.hapModulesInfo) {
      if (labelId === hapInfo.labelId) {
        moduleName = hapInfo.name;
      }
    }
    appName = await getContext(this).createModuleContext(moduleName).resourceManager.getStringValue(labelId);
    return appName;
  }

  replacePhotoPickerPreview(selectedMediaUri: string, replaceUri: string, callback: AsyncCallback<void>) {
    try {
      let fd = fs.openSync(replaceUri).fd;
      fs.close(fd);
    } catch (err) {
      callback({'code': 13900002, 'message': 'No such file', name: ''});
      return;
    }
    let date = Math.random();
    this.data = new Map([['CREATE_URI', [selectedMediaUri, replaceUri, date]]]);
    this.createCallbackMap.set(date, (grantUri: string, code: number, message: string) => {
      if (code !== 0) {
        callback({ 'name': '', 'code': code, 'message': message });
        return;
      }
      let createFd = 0;
      let replaceFd = 0;
      try {
        createFd = fs.openSync(grantUri, fs.OpenMode.READ_WRITE).fd;
        replaceFd = fs.openSync(replaceUri, fs.OpenMode.READ_ONLY).fd;
        fs.copyFileSync(replaceFd, createFd);
        this.data = new Map([['REPLACE_URI', [selectedMediaUri, grantUri, date]]]);
        this.replaceCallbackMap.set(date, callback);
      } catch (err) {
        callback({ 'code': 14000011, 'message': 'System inner fail', name: '' });
      } finally {
        fs.close(createFd);
        fs.close(replaceFd);
      }
    })
  }

  saveTrustedPhotoAssets(selectedMediaUris: Array<string>, callback: AsyncCallback<Array<string>>,
    config?: Array<photoAccessHelper.PhotoCreationConfig>, saveMode?: SaveMode) {
    if (!selectedMediaUris || selectedMediaUris.length === 0) {
      callback({'code': 14000002, 'message': 'Invalid URI', name: ''}, []);
      return;
    }
    this.getAppName().then((appName: string)=>{
      let date = Math.random();
      this.data = new Map([['SAVE_TRUSTED_PHOTO_ASSETS', [selectedMediaUris, config, saveMode, appName, date]]]);
      this.saveCallbackMap.set(date, callback);
    })
    console.info('PhotoPickerComponent SAVE_TRUSTED_PHOTO_ASSETS ');
  }

  saveTrustedPhotoAssetsEx(selectedMediaUris: Array<string>, settings?: Array<photoAccessHelper.CreationSetting>,
    saveMode?: SaveMode): Promise<Array<string>> {
    return new Promise((resolve, reject) => {
      if (!selectedMediaUris || selectedMediaUris.length === 0) {
        reject({ 'code': 14000002, 'message': 'Invalid URI', name: '' });
        return;
      }
      this.getAppName().then((appName: string) => {
        let date = Math.random();
        this.data = new Map([['SAVE_TRUSTED_PHOTO_ASSETS', [selectedMediaUris, settings, saveMode, appName, date]]]);
        this.saveCallbackPromises.set(date, [resolve, reject]);
      });
      console.info('PhotoPickerComponent SAVE_TRUSTED_PHOTO_ASSETS_EX');
    });
  }

  actionCreateCallback(grantUri: string, date: number, code: number, message: string) {
    if (this.createCallbackMap.has(date)) {
      let callback = this.createCallbackMap.get(date) as Function;
      if (callback) {
        callback(grantUri, code, message);
        this.createCallbackMap.delete(date);
      }
    }
  }

  actionReplaceCallback(date: number, err: BusinessError) {
    if (this.replaceCallbackMap.has(date)) {
      let callback = this.replaceCallbackMap.get(date) as Function;
      if (callback) {
        callback(err);
        this.replaceCallbackMap.delete(date);
      }
    }
  }

  actionSaveCallback(date: number, err: BusinessError, data: Array<string>) {
    if (this.saveCallbackMap.has(date)) {
      let callback = this.saveCallbackMap.get(date) as Function;
      if (callback) {
        callback(err, data);
        this.saveCallbackMap.delete(date);
      }
    }
    if (this.saveCallbackPromises.has(date)) {
      let promiseFunc = this.saveCallbackPromises.get(date) as [Function, Function];
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

  encrypt(data) {
    if (!data || data?.indexOf('file:///data/storage/') !== -1) {
      return '';
    }
    return data.replace(/(\/\w+)\./g, '/******.');
  }
}

export class PickerOptions extends photoAccessHelper.BaseSelectOptions {
  checkBoxColor?: string;
  backgroundColor?: string;
  isRepeatSelectSupported?: boolean;
  checkboxTextColor?: string;
  photoBrowserBackgroundColorMode?: PickerColorMode;
  maxSelectedReminderMode?: ReminderMode;
  orientation?: PickerOrientation;
  selectMode?: SelectMode;
  maxPhotoSelectNumber?: number;
  maxVideoSelectNumber?: number;
  isSlidingSelectionSupported?: boolean;
  photoBrowserCheckboxPosition?: [number, number];
  gridMargin?: Margin;
  photoBrowserMargin?: Margin;
  singleLineConfig?: SingleLineConfig;
  uiComponentColorMode?: PickerColorMode;
  combinedMediaTypeFilter?: Array<string>;
  pickerIndex?: number;
  preselectedInfos?: Array<PreselectedInfo>;
}

export class UpdatablePickerConfigs {
  mimeType?: photoAccessHelper.PhotoViewMIMETypes;
  mimeTypeFilter?: photoAccessHelper.MimeTypeFilter;
  maxSelectNumber?: number;
  maxPhotoSelectNumber?: number;
  maxVideoSelectNumber?: number;
  selectMode?: SelectMode;
  singleSelectionMode?: photoAccessHelper.SingleSelectionMode;
  isRepeatSelectSupported?: boolean;
  preselectedUris?: Array<string>;
  checkBoxColor?: string;
  checkboxTextColor?: string;
  backgroundColor?: string;
  photoBrowserBackgroundColorMode?: PickerColorMode,
  uiComponentColorMode?: PickerColorMode;
  appAlbumFilters?: Array<string>;
}

export class GridPinchMode {
  constructor() {
    this.gridPinchModeType = undefined;
    this.defaultGridLevel = GridLevel.STANDARD;
  }
}

export class BaseItemInfo {
  uri?: string;
  mimeType?: string;
  width?: number;
  height?: number;
  size?: number;
  duration?: number;
  movingPhotoBadgeState?: MovingPhotoBadgeStateType;
}

export class ItemInfo extends BaseItemInfo {
  itemType?: ItemType;
}

export class PhotoBrowserInfo {
  animatorParams?: AnimatorParams;
}

export class AnimatorParams {
  duration?: number;
  curve?: Curve | ICurve | string;
}

export class MaxSelected {
  data?: Map<MaxCountType, number>;
}

export class SingleLineConfig {
  itemDisplayRatio?: ItemDisplayRatio;
  itemBorderRadius?: Length | BorderRadiuses | LocalizedBorderRadiuses;
  itemGap?: Length;

  constructor() {
    this.itemDisplayRatio = ItemDisplayRatio.SQUARE_RATIO;
    this.itemBorderRadius = 0;
    this.itemGap = 0;
  }
}

class PhotoBrowserRangeInfo {
  uri?: string;
  photoBrowserRange?: PhotoBrowserRange;
}

class PhotoBrowserUIElementVisibility {
  elements?: Array<PhotoBrowserUIElement>;
  isVisible?: boolean;
}

export class BadgeConfig {
  badgeConfigType?: BadgeTypes,
  uris?: string[]
} 

class PreselectedInfo {
  uri:string;
  preselectablePickerIndex?: number = -1;
}

class ClickResult {
  uri: string;
  isSelected: boolean;
}

class LivePhotoMode {
  isAutoPlay: boolean;
  sceneType: SceneType;
}

export enum DataType {
  SET_SELECTED_URIS = 1,
  SET_ALBUM_URI = 2,
  SET_SELECTED_INFO = 3,
  SET_ITEM_CLICK_RESULT = 5
}

export enum GridPinchModeType {
  FULL_FUNCTION_GRID = 0,
}

export enum GridLevel {
  SPACIOUS = 0,
  STANDARD = 1,
  COMPACT = 2,
}

export enum ItemType {
  THUMBNAIL = 0,
  CAMERA = 1
}

export enum ClickType {
  SELECTED = 0,
  DESELECTED = 1
}

export enum PickerOrientation {
  VERTICAL = 0,
  HORIZONTAL = 1
}

export enum SelectMode {
  SINGLE_SELECT = 0,
  MULTI_SELECT = 1
}

export enum PickerColorMode {
  AUTO = 0,
  LIGHT = 1,
  DARK = 2
}

export enum ReminderMode {
  NONE = 0,
  TOAST = 1,
  MASK = 2
}

export enum MaxCountType {
  TOTAL_MAX_COUNT = 0,
  PHOTO_MAX_COUNT = 1,
  VIDEO_MAX_COUNT = 2
}

export enum PhotoBrowserRange {
  ALL = 0,
  SELECTED_ONLY = 1
}

export enum PhotoBrowserUIElement {
  CHECKBOX = 0,
  BACK_BUTTON = 1
}

export enum VideoPlayerState {
  PLAYING = 0,
  PAUSED = 1,
  STOPPED = 3,
  SEEK_START = 4,
  SEEK_FINISH = 5
}

export enum SaveMode {
  SAVE_AS = 0,
  OVERWRITE = 1
}

export enum ItemDisplayRatio {
  SQUARE_RATIO = 0,
  ORIGINAL_SIZE_RATIO = 1
}

export enum BadgeTypes {
  BADGE_UPLOADED = 1
}

export enum BadgeOptionType {
  SET_DATA = 1,
  ADD_DATA = 2,
  DELETE_DATA = 3
}

export enum SingleSelectMode {
  BROWSER_MODE = 0,
  SELECT_MODE = 1,
  BROWSER_AND_SELECT_MODE = 2
}

export enum MovingPhotoBadgeStateType {
  NOT_MOVING_PHOTO	= 0,
  MOVING_PHOTO_ENABLED	= 1,
  MOVING_PHOTO_DISABLED	= 2
}

export enum SceneType {
  GRID_TO_PHOTO_BROWSER = 0;
  PHOTO_BROWSER_SWIPE = 1;
}