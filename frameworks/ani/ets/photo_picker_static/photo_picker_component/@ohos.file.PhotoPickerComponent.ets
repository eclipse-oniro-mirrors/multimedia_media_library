/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

import {
  Component, Row, Column, Margin, Curve, ICurve, Length, BorderRadiuses, LocalizedBorderRadiuses,
  Builder, SecurityUIExtensionComponent, SecurityUIExtensionProxy
} from '@ohos.arkui.component';
import photoAccessHelper from '@ohos.file.photoAccessHelper';
import { fileIo as fs } from '@kit.CoreFileKit';
import { AsyncCallback, BusinessError } from '@kit.BasicServicesKit';
import { bundleManager } from '@kit.AbilityKit';
import { LengthMetrics } from '@kit.ArkUI';
import { LengthUnit } from '@ohos.arkui.node';
import { Observed, ObjectLink, Watch, State } from '@ohos.arkui.stateManagement';
import application from '@ohos.app.ability.application'
import { Context, UIContext } from '@ohos.arkui.UIContext';
import { resourceManager } from '@kit.LocalizationKit';
import { RecordData } from '@ohos.base';

const PHOTO_VIEW_MIME_TYPE_MAP = new Map<string, string>([
    ['*/*', 'FILTER_MEDIA_TYPE_ALL'],
    ['image/*', 'FILTER_MEDIA_TYPE_IMAGE'],
    ['video/*', 'FILTER_MEDIA_TYPE_VIDEO'],
    ['image/movingPhoto', 'FILTER_MEDIA_TYPE_IMAGE_MOVING_PHOTO']
])

function checkValueRetUndefined<T>(value: Object | undefined | null): T | undefined {
  return (value !== undefined && value !== null) ? (value as T) : undefined;
}

class LengthUnitUtils {
  private static instance: LengthUnitUtils = new LengthUnitUtils();

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

export type ItemsDeletedCallback = (baseItemInfos: BaseItemInfo[]) => void;

export type ExceedMaxSelectedCallback = (exceedMaxCountType: MaxCountType) => void;

export type CurrentAlbumDeletedCallback = () => void;

export type videoPlayStateChangedCallback = (state: VideoPlayerState) => void;

@Observed
export class PickerController {
  public data?: Map<string, Object>;
  private replaceCallbackMap: Map<number, Object> = new Map<number, Object>();
  private saveCallbackMap: Map<number, Object> = new Map<number, Object>();
  private createCallbackMap: Map<number, Object> = new Map<number, Object>();

  setData(type: DataType, data: Object): void {
    if (data === undefined) {
      return;
    }
    if (type === DataType.SET_SELECTED_URIS) {
      if (Array.isArray(data) && (data as string[]).every(item => typeof item === 'string')) {
        let uriLists: string[] = data as string[];
        if (uriLists) {
          this.data = new Map<string, Object>([['SET_SELECTED_URIS', [...uriLists]]]);
          console.info('PhotoPickerComponent SET_SELECTED_URIS' + this.encrypt(JSON.stringify(uriLists)));
        }
      }
    } else if (type === DataType.SET_ALBUM_URI) {
      let albumUri: string = data as string;
      if (albumUri !== undefined) {
        this.data = new Map<string, Object>([['SET_ALBUM_URI', albumUri]]);
        console.info('PhotoPickerComponent SET_ALBUM_URI' + this.encrypt(JSON.stringify(albumUri)));
      }
    } else {
      console.info('PhotoPickerComponent setData: other case');
    }
  }

  setMaxSelected(maxSelected: MaxSelected): void {
    if (maxSelected) {
      this.data = new Map<string, Object>([['SET_MAX_SELECT_COUNT', maxSelected]]);
      console.info('PhotoPickerComponent SET_MAX_SELECT_COUNT' + JSON.stringify(maxSelected));
    }
  }

  setPhotoBrowserItem(uri: string, photoBrowserRange?: PhotoBrowserRange): void {
    let photoBrowserRangeInfo: PhotoBrowserRangeInfo = new PhotoBrowserRangeInfo();
    photoBrowserRangeInfo.uri = uri;
    let newPhotoBrowserRange: PhotoBrowserRange = photoBrowserRange ? photoBrowserRange : PhotoBrowserRange.ALL;
    photoBrowserRangeInfo.photoBrowserRange = newPhotoBrowserRange;
    this.data = new Map<string, Object>([['SET_PHOTO_BROWSER_ITEM', photoBrowserRangeInfo]]);
    console.info('PhotoPickerComponent SET_PHOTO_BROWSER_ITEM' + this.encrypt(JSON.stringify(photoBrowserRangeInfo)));
  }

  exitPhotoBrowser(): void {
    this.data = new Map<string, Object>([['EXIT_PHOTO_BROWSER', true]]);
    console.info('PhotoPickerComponent EXIT_PHOTO_BROWSER');
  }

  setPhotoBrowserUIElementVisibility(elements: PhotoBrowserUIElement[], isVisible: boolean): void {
    let photoBrowserUIElementVisibility: PhotoBrowserUIElementVisibility = new PhotoBrowserUIElementVisibility();
    photoBrowserUIElementVisibility.elements = elements;
    photoBrowserUIElementVisibility.isVisible = isVisible;
    this.data = new Map<string, Object>([['SET_PHOTO_BROWSER_UI_ELEMENT_VISIBILITY', photoBrowserUIElementVisibility]]);
    console.info('PhotoPickerComponent SET_PHOTO_BROWSER_UI_ELEMENT_VISIBILITY ' +
      JSON.stringify(photoBrowserUIElementVisibility));
  }

  private async getAppName(): Promise<string> {
    let flags: int = bundleManager.BundleFlag.GET_BUNDLE_INFO_WITH_ABILITY | // for appName
    bundleManager.BundleFlag.GET_BUNDLE_INFO_WITH_HAP_MODULE | // for appName
    bundleManager.BundleFlag.GET_BUNDLE_INFO_WITH_SIGNATURE_INFO | // for appId
    bundleManager.BundleFlag.GET_BUNDLE_INFO_WITH_APPLICATION; // for appInfo
    let bundleInfo: bundleManager.BundleInfo = bundleManager.getBundleInfoForSelfSync(flags);
    let labelId: long = bundleInfo.appInfo!.labelId;
    let appName: string = '';
    let moduleName: string = '';
    for (let hapInfo of bundleInfo.hapModulesInfo) {
      if (labelId === hapInfo.labelId) {
        moduleName = hapInfo.name;
      }
    }
    let uiContext: UIContext | undefined = UIContext.getCallingScopeUIContext();
    if (!uiContext || !uiContext.isAvailable()) {
      console.warn('UIContext is not available, using default instance');
      const allContexts: UIContext[] = UIContext.getAllUIContexts();
      if (allContexts.length > 0) {
        uiContext = allContexts[0];
      } else {
        uiContext = new UIContext();
      }
    }
    let appContext: Context | undefined = uiContext?.getHostContext();
    if (appContext != undefined) {
      let moduleContext: Context = await application.createModuleContext(appContext!, moduleName);
      appName = await moduleContext.resourceManager.getStringValue(labelId);
    }
    return appName;
  }

  replacePhotoPickerPreview(selectedMediaUri: string, replaceUri: string, callback: AsyncCallback<void>): void {
    try {
      let fd = fs.openSync(replaceUri).fd;
      fs.close(fd);
    } catch (err) {
      callback({'code': 13900002, 'message': 'No such file', 'name': ''} as BusinessError, undefined);
      return;
    }
    let date = Math.random();
    this.data = new Map<string, Object>([['CREATE_URI', [selectedMediaUri, replaceUri, date]]]);
    this.createCallbackMap.set(date, (grantUri: string, code: int, message: string) => {
      if (code !== 0) {
        callback({ 'name': '', 'code': code, 'message': message } as BusinessError, undefined);
        return;
      }
      let createFd = 0;
      let replaceFd = 0;
      try {
        createFd = fs.openSync(grantUri, fs.OpenMode.READ_WRITE).fd;
        replaceFd = fs.openSync(replaceUri, fs.OpenMode.READ_ONLY).fd;
        fs.copyFileSync(replaceFd, createFd);
        this.data = new Map<string, Object>([['REPLACE_URI', [selectedMediaUri, grantUri, date]]]);
        this.replaceCallbackMap.set(date, callback);
      } catch (err) {
        callback({ 'code': 14000011, 'message': 'System inner fail', 'name': '' } as BusinessError, undefined);
      } finally {
        fs.close(createFd);
        fs.close(replaceFd);
      }
    })
  }

  saveTrustedPhotoAssets(selectedMediaUris: string[], callback: AsyncCallback<string[]>,
    config?: photoAccessHelper.PhotoCreationConfig[], saveMode?: SaveMode): void {
    if (!selectedMediaUris || selectedMediaUris.length === 0) {
      callback({'code': 14000002, 'message': 'Invalid URI', 'name': ''}, []);
      return;
    }
    this.getAppName().then((appName: string)=>{
      let date = Math.random();
      this.data = new Map<string, Object>([['SAVE_TRUSTED_PHOTO_ASSETS', [selectedMediaUris, config, saveMode, appName, date]]]);
      this.saveCallbackMap.set(date, callback);
    })
    console.info('PhotoPickerComponent SAVE_TRUSTED_PHOTO_ASSETS ');
  }

  actionCreateCallback(grantUri: string, date: number, code: int, message: string): void {
    if (this.createCallbackMap.has(date)) {
      let callback = this.createCallbackMap.get(date) as ((grantUri: string, code: int, message: string) => void);
      if (callback) {
        callback(grantUri, code, message);
        this.createCallbackMap.delete(date);
      }
    }
  }

  actionReplaceCallback(date: number, err: BusinessError): void {
    if (this.replaceCallbackMap.has(date)) {
      let callback = this.replaceCallbackMap.get(date) as ((err: BusinessError) => void);
      if (callback) {
        callback(err);
        this.replaceCallbackMap.delete(date);
      }
    }
  }

  actionSaveCallback(date: number, err: BusinessError, data: string[]): void {
    if (this.saveCallbackMap.has(date)) {
      let callback = this.saveCallbackMap.get(date) as ((err: BusinessError, data: string[]) => void);
      if (callback) {
        callback(err, data);
        this.saveCallbackMap.delete(date);
      }
    }
  }

  encrypt(data: string | undefined): string {
    if (!data || data.indexOf('file:///data/storage/') !== -1) {
      return '';
    }
    let regexp: RegExp = new RegExp('(\\/\\w+)\\.', 'g');
    return data.replace(regexp, '/******.');
  }
}

export class PickerOptions extends photoAccessHelper.BaseSelectOptions {
  public checkBoxColor?: string;
  public backgroundColor?: string;
  public isRepeatSelectSupported?: boolean;
  public checkboxTextColor?: string;
  public photoBrowserBackgroundColorMode?: PickerColorMode;
  public maxSelectedReminderMode?: ReminderMode;
  public orientation?: PickerOrientation;
  public selectMode?: SelectMode;
  public maxPhotoSelectNumber?: int;
  public maxVideoSelectNumber?: int;
  public isSlidingSelectionSupported?: boolean;
  public photoBrowserCheckboxPosition?: [double, double];
  public gridMargin?: Margin;
  public photoBrowserMargin?: Margin;
  public singleLineConfig?: SingleLineConfig;
  public uiComponentColorMode?: PickerColorMode;
  public isPreviewForSingleSelectionSupported?: boolean = undefined;
  public singleSelectionMode?: photoAccessHelper.SingleSelectionMode = undefined;
  public gridStartOffset?: int;
  public gridEndOffset?: int;
}

export class BaseItemInfo {
  public uri?: string;
  public mimeType?: string;
  public width?: int;
  public height?: int;
  public size?: int;
  public duration?: int;
}

export class ItemInfo extends BaseItemInfo {
  public itemType?: ItemType;
}

export class PhotoBrowserInfo {
  public animatorParams?: AnimatorParams;
}

export class AnimatorParams {
  public duration?: int;
  public curve?: Curve | ICurve | string;
}

export class MaxSelected {
  public data?: Map<MaxCountType, int>;
}

export class SingleLineConfig {
  public itemDisplayRatio?: ItemDisplayRatio;
  public itemBorderRadius?: Length | BorderRadiuses | LocalizedBorderRadiuses;
  public itemGap?: Length;

  constructor() {
    this.itemDisplayRatio = ItemDisplayRatio.SQUARE_RATIO;
    this.itemBorderRadius = 0;
    this.itemGap = 0;
  }
}

class PhotoBrowserRangeInfo {
  public uri?: string;
  public photoBrowserRange?: PhotoBrowserRange;
}

class PhotoBrowserUIElementVisibility {
  public elements?: PhotoBrowserUIElement[];
  public isVisible?: boolean;
}

export enum DataType {
  SET_SELECTED_URIS = 1,
  SET_ALBUM_URI = 2
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
  STOPPED = 2,
  SEEK_START = 3,
  SEEK_FINISH = 4
}

export enum SaveMode {
  SAVE_AS = 0,
  OVERWRITE = 1
}

export enum ItemDisplayRatio {
  SQUARE_RATIO = 0,
  ORIGINAL_SIZE_RATIO = 1
}
@Component
export struct PhotoPickerComponent {
  pickerOptions?: PickerOptions | undefined;
  onSelect?: (uri: string) => void;
  onDeselect?: (uri: string) => void;
  onItemClicked?: (itemInfo: ItemInfo, clickType: ClickType) => boolean;
  onEnterPhotoBrowser?: (photoBrowserInfo: PhotoBrowserInfo) => boolean;
  onExitPhotoBrowser?: (photoBrowserInfo: PhotoBrowserInfo) => boolean;
  onPickerControllerReady?: () => void;
  onPhotoBrowserChanged?: (browserItemInfo: BaseItemInfo) => boolean;
  onSelectedItemsDeleted?: ItemsDeletedCallback;
  onExceedMaxSelected?: ExceedMaxSelectedCallback;
  onCurrentAlbumDeleted?: CurrentAlbumDeletedCallback;
  onVideoPlayStateChanged?: videoPlayStateChangedCallback;
  @ObjectLink @Watch('onChanged') pickerController: PickerController = new PickerController();
  private proxy: SecurityUIExtensionProxy | undefined;
  @State revokeIndex: int = 0;

  onChanged(propName: string): void {
    if (!this.proxy) {
      return;
    }
    let data: Map<string, Object> | undefined = this.pickerController?.data;
    if (data?.has('SET_SELECTED_URIS')) {
      this.proxy?.send({ 'selectUris': data?.get('SET_SELECTED_URIS') as string[] } as Record<string, RecordData>);
      console.info('PhotoPickerComponent onChanged: SET_SELECTED_URIS');
    } else if (data?.has('SET_ALBUM_URI')) {
      this.proxy?.send({ 'albumUri': data?.get('SET_ALBUM_URI') as string } as Record<string, RecordData>);
      console.info('PhotoPickerComponent onChanged: SET_ALBUM_URI');
    } else if (data?.has('SET_MAX_SELECT_COUNT')) {
      this.onSetMaxSelectCount(data);
    } else if (data?.has('SET_PHOTO_BROWSER_ITEM')) {
      this.onSetPhotoBrowserItem(data);
    } else {
      this.otherOnChange(data);
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
    } else {
      console.info('PhotoPickerComponent onChanged: other case');
    }
  }

  private onSetMaxSelectCount(data?: Map<string, Object>): void {
    let maxSelected: MaxSelected = data?.get('SET_MAX_SELECT_COUNT') as MaxSelected;
    let map: Map<MaxCountType, int> | undefined = maxSelected?.data;
    this.proxy?.send({
      'totalCount': map?.get(MaxCountType.TOTAL_MAX_COUNT),
      'photoCount': map?.get(MaxCountType.PHOTO_MAX_COUNT),
      'videoCount': map?.get(MaxCountType.VIDEO_MAX_COUNT)
    } as Record<string, RecordData>);
    console.info('PhotoPickerComponent onChanged: SET_MAX_SELECT_COUNT');
  }

  private onSetPhotoBrowserItem(data?: Map<string, Object>): void {
    let photoBrowserRangeInfo: PhotoBrowserRangeInfo = data?.get('SET_PHOTO_BROWSER_ITEM') as PhotoBrowserRangeInfo;
    this.proxy?.send({
      'itemUri': photoBrowserRangeInfo?.uri,
      'photoBrowserRange': photoBrowserRangeInfo?.photoBrowserRange
    } as Record<string, RecordData>);
    console.info('PhotoPickerComponent onChanged: SET_PHOTO_BROWSER_ITEM');
  }

  private handleExitPhotoBrowser(): void {
    this.proxy?.send({ 'exitPhotoBrowser': true } as Record<string, RecordData>);
    console.info('PhotoPickerComponent onChanged: EXIT_PHOTO_BROWSER');
  }

  private onSetPhotoBrowserUIElementVisibility(data?: Map<string, Object>): void {
    let photoBrowserUIElementVisibility: PhotoBrowserUIElementVisibility =
      data?.get('SET_PHOTO_BROWSER_UI_ELEMENT_VISIBILITY') as PhotoBrowserUIElementVisibility;
    this.proxy?.send({
      'elements': photoBrowserUIElementVisibility?.elements,
      'isVisible': photoBrowserUIElementVisibility?.isVisible
    } as Record<string, RecordData>);
    console.info('PhotoPickerComponent onChanged: SET_PHOTO_BROWSER_UI_ELEMENT_VISIBILITY');
  }

  private onCreateUri(data?: Map<string, Object>): void {
    let array: Object[] = data?.get('CREATE_URI') as Object[];
    this.proxy?.send({
      'selectedMediaUri': array[0],
      'createUri': array[1],
      'date': array[2]
    } as Record<string, RecordData>);
    console.info('PhotoPickerComponent onChanged CREATE_URI');
  }

  private onReplaceUri(data?: Map<string, Object>): void {
    let array: Object[] = data?.get('REPLACE_URI') as Object[];
    this.proxy?.send({
      'oriUri': array[0],
      'replaceUri': array[1],
      'date': array[2]
    } as Record<string, RecordData>);
    console.info('PhotoPickerComponent onChanged REPLACE_URI');
  }

  private onSaveTrustedPhotoAssets(data?: Map<string, Object>): void {
    let array: object[] = data?.get('SAVE_TRUSTED_PHOTO_ASSETS') as object[];
    this.proxy?.send({
      'replaceUris': array[0],
      'config': array[1],
      'saveMode': array[2],
      'appName': array[3],
      'date': array[4]
    } as Record<string, RecordData>);
    console.info('PhotoPickerComponent onChanged SAVE_REPLACE_PHOTO_ASSETS');
  }

  private handleSelectOrDeselect(wantParam: Record<string, RecordData>): void {
    let isSelect: boolean = wantParam['isSelect'] as boolean;
    if (isSelect) {
      if (this.onSelect) {
        this.onSelect!(wantParam['select-item-list'] as string);
        console.info('PhotoPickerComponent onReceive: onSelect');
      }
    } else {
      if (this.onDeselect) {
        this.onDeselect!(wantParam['select-item-list'] as string);
        console.info('PhotoPickerComponent onReceive: onDeselect');
      }
    }
  }

  private handleItemClick(wantParam: Record<string, RecordData>): void {
    if (this.onItemClicked) {
      let clickType: ClickType = ClickType.SELECTED;
      let type: string = wantParam['clickType'] as string;
      if (type === 'select') {
        clickType = ClickType.SELECTED;
      } else if (type === 'deselect') {
        clickType = ClickType.DESELECTED;
      } else {
        console.info('PhotoPickerComponent onReceive: other clickType');
      }
      let itemInfo: ItemInfo = new ItemInfo();
      let itemType: string = wantParam['itemType'] as string;
      if (itemType === 'thumbnail') {
        itemInfo.itemType = ItemType.THUMBNAIL;
      } else if (itemType === 'camera') {
        itemInfo.itemType = ItemType.CAMERA;
      } else {
        console.info('PhotoPickerComponent onReceive: other itemType');
      }
      itemInfo.uri = checkValueRetUndefined<string>(wantParam['uri']);
      itemInfo.mimeType = checkValueRetUndefined<string>(wantParam['mimeType']);
      itemInfo.width = checkValueRetUndefined<int>(wantParam['width']);
      itemInfo.height = checkValueRetUndefined<int>(wantParam['height']);
      itemInfo.size = checkValueRetUndefined<int>(wantParam['size']);
      itemInfo.duration = checkValueRetUndefined<int>(wantParam['duration']);
      let result: boolean = this.onItemClicked!(itemInfo, clickType);
      console.info('PhotoPickerComponent onReceive: onItemClicked = ' + clickType);
      if (this.proxy) {
        if (itemType === 'thumbnail' && clickType === ClickType.SELECTED) {
          this.proxy?.send({ 'clickConfirm': itemInfo.uri, 'isConfirm': result } as Record<string, RecordData>);
          console.info('PhotoPickerComponent onReceive: click confirm: uri = ' +
            this.pickerController.encrypt(itemInfo.uri) + 'isConfirm = ' + result);
        }
        if (itemType === 'camera') {
          this.proxy?.send({ 'enterCamera': result } as Record<string, RecordData>);
          console.info('PhotoPickerComponent onReceive: enter camera ' + result);
        }
      }
    }
  }

  private handleEnterOrExitPhotoBrowser(wantParam: Record<string, RecordData>): void {
    let isEnter: boolean = wantParam['isEnter'] as boolean;
    let photoBrowserInfo: PhotoBrowserInfo = new PhotoBrowserInfo();
    photoBrowserInfo.animatorParams = new AnimatorParams();
    photoBrowserInfo.animatorParams!.duration = checkValueRetUndefined<int>(wantParam['duration']);
    photoBrowserInfo.animatorParams!.curve = checkValueRetUndefined<Curve | ICurve | string>(wantParam['curve']);
    if (isEnter) {
      if (this.onEnterPhotoBrowser) {
        this.onEnterPhotoBrowser!(photoBrowserInfo);
      }
    } else {
      if (this.onExitPhotoBrowser) {
        this.onExitPhotoBrowser!(photoBrowserInfo);
      }
    }
    console.info('PhotoPickerComponent onReceive: onPhotoBrowserStateChanged = ' + isEnter);
  }

  private handleOtherOnReceive(wantParam: Record<string, RecordData>): void {
    let dataType: string = wantParam['dataType'] as string;
    if (dataType === 'exceedMaxSelected') {
      if (this.onExceedMaxSelected) {
        this.onExceedMaxSelected!(wantParam['maxCountType'] as MaxCountType);
      }
    } else if (dataType === 'selectedItemsDeleted') {
      if (this.onSelectedItemsDeleted) {
        this.onSelectedItemsDeleted!(wantParam['selectedItemInfos'] as BaseItemInfo[]);
      }
    } else if (dataType === 'currentAlbumDeleted') {
      if (this.onCurrentAlbumDeleted) {
        this.onCurrentAlbumDeleted!();
      }
    } else {
      console.info('PhotoPickerComponent onReceive: other case');
    }
  }

  private handlePhotoBrowserChange(wantParam: Record<string, RecordData>): void {
    let browserItemInfo: BaseItemInfo = new BaseItemInfo();
    browserItemInfo.uri = wantParam['uri'] as string;
    if (this.onPhotoBrowserChanged) {
      this.onPhotoBrowserChanged!(browserItemInfo);
    }
    console.info('PhotoPickerComponent onReceive: onPhotoBrowserChanged = ' +
      this.pickerController.encrypt(browserItemInfo.uri));
  }

  private handleVideoPlayStateChange(wantParam: Record<string, RecordData>): void {
    if (this.onVideoPlayStateChanged) {
      this.onVideoPlayStateChanged!(wantParam['state'] as VideoPlayerState)
    }
    console.info('PhotoPickerComponent onReceive: onVideoPlayStateChanged = ' + JSON.stringify(wantParam));
  }

  private handleCreateCallback(wantParam: Record<string, RecordData>): void {
    this.pickerController.actionCreateCallback(wantParam['grantUri'] as string, wantParam['date'] as number,
      wantParam['code'] as int, wantParam['message'] as string);
    console.info('PhotoPickerComponent onReceive: handleCreateCallback');
  }

  private handleReplaceCallback(wantParam: Record<string, RecordData>): void {
    this.pickerController.actionReplaceCallback(wantParam['date'] as number,
      { 'name': '', 'code': wantParam['code'] as int, 'message': wantParam['message'] as string });
    console.info('PhotoPickerComponent onReceive: handleReplaceCallback');
  }

  private handleSaveCallback(wantParam: Record<string, RecordData>): void {
    this.pickerController.actionSaveCallback(wantParam['date'] as number,
      { 'name': '', 'code': wantParam['code'] as int, 'message': wantParam['error'] as string },
      wantParam['data'] as string[]);
    console.info('PhotoPickerComponent onReceive: handleSaveCallback');
  }

  private handleOnReceive(wantParam: Record<string, RecordData>): void {
    let dataType: string = wantParam['dataType'] as string;
    console.info('PhotoPickerComponent onReceive: dataType = ' + dataType);
    if (dataType === 'selectOrDeselect') {
      this.handleSelectOrDeselect(wantParam);
    } else if (dataType === 'itemClick') {
      this.handleItemClick(wantParam);
    } else if (dataType === 'onPhotoBrowserStateChanged') {
      this.handleEnterOrExitPhotoBrowser(wantParam);
    } else if (dataType === 'remoteReady') {
      if (this.onPickerControllerReady) {
        this.onPickerControllerReady!();
        console.info('PhotoPickerComponent onReceive: onPickerControllerReady');
      }
    } else if (dataType === 'onPhotoBrowserChanged') {
      this.handlePhotoBrowserChange(wantParam);
    } else if (dataType === 'onVideoPlayStateChanged') {
      this.handleVideoPlayStateChange(wantParam)
    } else if (dataType === 'replaceCallback') {
      this.handleReplaceCallback(wantParam);
    } else if (dataType === 'createCallback') {
      this.handleCreateCallback(wantParam);
    } else if (dataType === 'saveCallback') {
      this.handleSaveCallback(wantParam);
    } else if (dataType === 'onBackground') {
      console.info('PhotoPickerComponent onReceive: onBackground');
      this.revokeIndex = 0;
    } else {
      this.handleOtherOnReceive(wantParam);
      console.info('PhotoPickerComponent onReceive: other case');
    }
    console.info('PhotoPickerComponent onReceive' + this.pickerController.encrypt(JSON.stringify(wantParam)));
  }

  parseMimeTypeFilter(filter?: photoAccessHelper.MimeTypeFilter): object | undefined {
    if (!filter) {
      return undefined;
    }
    let mimeTypeFilterObj: photoAccessHelper.MimeTypeFilter = {
      mimeTypeArray: [],
    };
    if (filter.mimeTypeArray) {
      for (let mimeType of filter.mimeTypeArray) {
        if (PHOTO_VIEW_MIME_TYPE_MAP.has(mimeType)) {
          mimeTypeFilterObj.mimeTypeArray.push(PHOTO_VIEW_MIME_TYPE_MAP.get(mimeType)!);
        } else {
          mimeTypeFilterObj.mimeTypeArray.push(mimeType);
        }
      }
    }
    return mimeTypeFilterObj;
  }

  private convertMIMETypeToFilterType(mimeType: photoAccessHelper.PhotoViewMIMETypes | undefined): string {
    let filterType: string = '';
    if (mimeType === undefined) {
      return filterType;
    }
    if (PHOTO_VIEW_MIME_TYPE_MAP.has(mimeType)) {
      filterType = PHOTO_VIEW_MIME_TYPE_MAP.get(mimeType!) ?? '';
    } else {
      filterType = PHOTO_VIEW_MIME_TYPE_MAP.get('*/*') ?? '';
    }
    console.info('PhotoPickerComponent convertMIMETypeToFilterType: ' + JSON.stringify(filterType));
    return filterType;
  }

  private getSingleLineConfig(singleLineConfig: SingleLineConfig | undefined): SingleLineConfig | undefined {
    if (singleLineConfig === null || singleLineConfig === undefined) {
      return undefined;
    }
    singleLineConfig!.itemDisplayRatio = (singleLineConfig!.itemDisplayRatio === null ||
      singleLineConfig!.itemDisplayRatio === undefined) ? ItemDisplayRatio.SQUARE_RATIO :
      singleLineConfig!.itemDisplayRatio;
    singleLineConfig!.itemBorderRadius = this.getSingleLineConfigItemBorderRadius(singleLineConfig!.itemBorderRadius);
    singleLineConfig!.itemGap = this.getLength(singleLineConfig!.itemGap);
    return singleLineConfig
  }

  private getSingleLineConfigItemBorderRadius(itemBorderRadius?: Length | BorderRadiuses |
    LocalizedBorderRadiuses): Length | BorderRadiuses | LocalizedBorderRadiuses {
    if (itemBorderRadius === undefined || itemBorderRadius === null) {
      return 0;
    }
    if (typeof itemBorderRadius === 'number' || typeof itemBorderRadius === 'string') {
      return itemBorderRadius!;
    }
    if (this.hasOwnProp(itemBorderRadius!, ['topStart', 'topEnd', 'bottomStart', 'bottomEnd'])) {
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
    if (this.hasOwnProp(itemBorderRadius!, ['topLeft', 'topRight', 'bottomLeft', 'bottomRight'])) {
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
    const borderRadiusesValue = itemBorderRadius as resourceManager.Resource;
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
      return prop!;
    }
    const resource = LengthMetrics.resource(prop! as resourceManager.Resource);
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

  @Builder
  build(): void {
    Row() {
      Column() {
        SecurityUIExtensionComponent({
          parameters: {
            'errorRevokeIndex': this.revokeIndex,
            'ability.want.params.uiExtensionTargetType': 'photoPicker',
            'uri': 'multipleselect',
            'targetPage': 'photoPage',
            'filterMediaType': this.convertMIMETypeToFilterType(this.pickerOptions?.MIMEType),
            'mimeTypeFilter': this.parseMimeTypeFilter(this.pickerOptions?.mimeTypeFilter),
            'fileSizeFilter': this.pickerOptions?.fileSizeFilter,
            'videoDurationFilter': this.pickerOptions?.videoDurationFilter,
            'maxSelectNumber': this.pickerOptions?.maxSelectNumber,
            'isPhotoTakingSupported': this.pickerOptions?.isPhotoTakingSupported,
            'isEditSupported': false,
            'recommendationOptions': this.pickerOptions?.recommendationOptions,
            'preselectedUri': this.pickerOptions?.preselectedUris,
            'isFromPickerView': true,
            'isNeedActionBar': false,
            'isNeedSelectBar': false,
            'isSearchSupported': this.pickerOptions?.isSearchSupported,
            'checkBoxColor': this.pickerOptions?.checkBoxColor,
            'backgroundColor': this.pickerOptions?.backgroundColor,
            'checkboxTextColor': this.pickerOptions?.checkboxTextColor,
            'photoBrowserBackgroundColorMode': this.pickerOptions?.photoBrowserBackgroundColorMode,
            'isRepeatSelectSupported': this.pickerOptions?.isRepeatSelectSupported,
            'maxSelectedReminderMode': this.pickerOptions?.maxSelectedReminderMode,
            'orientation': this.pickerOptions?.orientation,
            'selectMode': this.pickerOptions?.selectMode,
            'maxPhotoSelectNumber': this.pickerOptions?.maxPhotoSelectNumber,
            'maxVideoSelectNumber': this.pickerOptions?.maxVideoSelectNumber,
            'isOnItemClickedSet': this.onItemClicked ? true : false,
            'isPreviewForSingleSelectionSupported': this.pickerOptions?.isPreviewForSingleSelectionSupported,
            'singleSelectionMode': this.pickerOptions?.singleSelectionMode,
            'isSlidingSelectionSupported': this.pickerOptions?.isSlidingSelectionSupported,
            'photoBrowserCheckboxPosition': this.pickerOptions?.photoBrowserCheckboxPosition,
            'gridMargin': this.pickerOptions?.gridMargin,
            'photoBrowserMargin': this.pickerOptions?.photoBrowserMargin,
            'gridStartOffset': this.pickerOptions?.gridStartOffset,
            'gridEndOffset': this.pickerOptions?.gridEndOffset,
            'singleLineConfig': this.getSingleLineConfig(this.pickerOptions?.singleLineConfig),
            'uiComponentColorMode': this.pickerOptions?.uiComponentColorMode,
          } as Record<string, RecordData>
        })
          .height('100%')
          .width('100%')
          .onRemoteReady((proxy: SecurityUIExtensionProxy) => {
            this.proxy = proxy;
            console.info('PhotoPickerComponent onRemoteReady');
          })
          .onReceive((data: Record<string, RecordData>) => {
            this.handleOnReceive(data);
          })
          .onError((error: BusinessError) => {
            console.info('PhotoPickerComponent onError: ' + JSON.stringify(error));
            console.info('PhotoPickerComponent revokeIndex: ' + this.revokeIndex);
            if (error.code === 100014 && this.revokeIndex < 3) {
              this.revokeIndex++;
            }
          });
      }
      .width('100%')
    }
    .height('100%')
  }
}