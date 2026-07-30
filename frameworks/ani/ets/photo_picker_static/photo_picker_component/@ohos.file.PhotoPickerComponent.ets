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
  Builder, SecurityUIExtensionComponent, SecurityUIExtensionProxy, EdgeEffect
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

const PARAMETERS_VALIDATE_FAILED_MESSAGE =
  'Scene parameters validate failed, possible causes:' +
  '  1. An invalid enumeration value was passed. Only MOVING_PHOTO_ENABLE and' +
  '  MOVING_PHOTO_DISABLE are supported for configuration;';
const ILLEGAL_SCENARIO_CALL_ERROR_MESSAGE =
  'Invalid call context. Possible causes:' +
  '  1. The API is called outside the photo browsing scenario.' +
  '  2. The API is called when isMovingPhotoBadgeShown is already set to true.';
const PARAMETERS_VALIDATE_FAILED_CODE = 23800151;
const ILLEGAL_SCENARIO_CALL_ERROR_CODE = 23800202;
const MAX_BADGE_CONFIG_SIZE = 200000;
const BATCH_BADGE_CONFIG_SIZE = 1000;
const BATCH_PRESELECTED_INFOS = 200;

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
  private saveCallbackPromises: Map<number, Object> = new Map<number, Object>();
  private completedPromises: Map<number, Object> = new Map<number, Object>();
  public browserPropsRegistry?: () => BrowserProps;

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
    } else if (type === DataType.SET_SELECTED_INFO) {
      if (Array.isArray(data)) {
        let uriAndPickerIndexLists: PreselectedInfo[] = data as PreselectedInfo[];
        this.data = new Map<string, Object>([['SET_SELECTED_INFO', uriAndPickerIndexLists]]);
        console.info('PhotoPickerComponent SET_SELECTED_INFO' + this.encrypt(JSON.stringify(uriAndPickerIndexLists)));
      }
    } else if (type === DataType.SET_BADGE_CONFIGS) {
      let badgeConfig: BadgeConfig = data as BadgeConfig;
      this.data = new Map<string, Object>([
        ['SET_BADGE_CONFIGS', badgeConfig], ['BADGE_CONFIGS_OPTION_TYPE', BadgeOptionType.SET_DATA]
      ]);
      console.info('PhotoPickerComponent SET_BADGE_CONFIGS');
    } else {
      console.info('PhotoPickerComponent setData: other case');
    }
  }

  addData(type: DataType, data: Object): void {
    if (data === undefined) {
      return;
    }
    if (type === DataType.SET_BADGE_CONFIGS) {
      let badgeConfig: BadgeConfig = data as BadgeConfig;
      this.data = new Map<string, Object>([
        ['SET_BADGE_CONFIGS', badgeConfig], ['BADGE_CONFIGS_OPTION_TYPE', BadgeOptionType.ADD_DATA]
      ]);
      console.info('PhotoPickerComponent addData SET_BADGE_CONFIGS');
    } else if (type === DataType.SET_ITEM_CLICK_RESULT) {
      if (Array.isArray(data)) {
        let clickResults: ClickResult[] = data as ClickResult[];
        this.data = new Map<string, Object>([['SET_ITEM_CLICK_RESULT', clickResults]]);
        console.info('PhotoPickerComponent addData SET_ITEM_CLICK_RESULT');
      }
    } else {
      console.info('PhotoPickerComponent addData: other case');
    }
  }

  deleteData(type: DataType, data: Object): void {
    if (data === undefined) {
      return;
    }
    if (type === DataType.SET_BADGE_CONFIGS) {
      let badgeConfig: BadgeConfig = data as BadgeConfig;
      this.data = new Map<string, Object>([
        ['SET_BADGE_CONFIGS', badgeConfig], ['BADGE_CONFIGS_OPTION_TYPE', BadgeOptionType.DELETE_DATA]
      ]);
      console.info('PhotoPickerComponent deleteData SET_BADGE_CONFIGS');
    } else {
      console.info('PhotoPickerComponent deleteData: other case');
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

  saveTrustedPhotoAssetsEx(selectedMediaUris: string[], settings?: photoAccessHelper.CreationSetting[],
    saveMode?: SaveMode): Promise<string[]> {
    return new Promise<string[]>((resolve: (v: string[]) => void, reject: (e: BusinessError) => void) => {
      if (!selectedMediaUris || selectedMediaUris.length === 0) {
        reject({ 'code': 14000002, 'message': 'Invalid URI', 'name': '' } as BusinessError);
        return;
      }
      this.getAppName().then((appName: string) => {
        let date = Math.random();
        this.data = new Map<string, Object>([
          ['SAVE_TRUSTED_PHOTO_ASSETS', [selectedMediaUris, settings, saveMode, appName, date]]
        ]);
        this.saveCallbackPromises.set(date, [resolve, reject]);
      });
      console.info('PhotoPickerComponent SAVE_TRUSTED_PHOTO_ASSETS_EX');
    });
  }

  updatePickerOptions(updateConfig: UpdatablePickerConfigs): Promise<void> {
    return new Promise<void>((resolve: (v: undefined) => void, reject: (e: BusinessError) => void) => {
      if (updateConfig !== undefined) {
        this.data = new Map<string, Object>([['UPDATE_CONFIG', updateConfig]]);
        console.info('PhotoPickerComponent UPDATE_CONFIG');
      }
      resolve(undefined);
    });
  }

  setMovingPhotoState(movingPhotoState: photoAccessHelper.MovingPhotoBadgeStateType): Promise<void> {
    return new Promise<void>((resolve: (v: undefined) => void, reject: (e: BusinessError) => void) => {
      if (movingPhotoState !== photoAccessHelper.MovingPhotoBadgeStateType.MOVING_PHOTO_ENABLED &&
        movingPhotoState !== photoAccessHelper.MovingPhotoBadgeStateType.MOVING_PHOTO_DISABLED) {
        reject({
          'code': PARAMETERS_VALIDATE_FAILED_CODE, 'message': PARAMETERS_VALIDATE_FAILED_MESSAGE, 'name': ''
        } as BusinessError);
        return;
      }
      let browserProps: BrowserProps = this.browserPropsRegistry ? this.browserPropsRegistry!() : new BrowserProps();
      console.info('SET_MOVINGPHOTO_STATE, isPhotoBrowserShow: ' + browserProps.isPhotoBrowserShow +
        ', isMovingPhotoBadgeShownValid=' + browserProps.isMovingPhotoBadgeShownValid);
      if (!browserProps.isPhotoBrowserShow || browserProps.isMovingPhotoBadgeShownValid) {
        reject({
          'code': ILLEGAL_SCENARIO_CALL_ERROR_CODE, 'message': ILLEGAL_SCENARIO_CALL_ERROR_MESSAGE, 'name': ''
        } as BusinessError);
        return;
      }
      this.data = new Map<string, Object>([['SET_MOVINGPHOTO_STATE', movingPhotoState]]);
      console.info('PhotoPickerComponent SET_MOVINGPHOTO_STATE: ' + movingPhotoState);
      resolve(undefined);
    });
  }

  completed(): Promise<CompletedResult> {
    return new Promise<CompletedResult>((resolve: (result: CompletedResult) => void,
      reject: (reason: BusinessError) => void) => {
      let date: number = Math.random();
      this.data = new Map<string, Object>([['COMPLETED', new CompletedResult()], ['COMPLETE_DATE', date]]);
      this.completedPromises.set(date, [resolve, reject]);
      console.info('PhotoPickerComponent COMPLETED');
    });
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
    if (this.saveCallbackPromises.has(date)) {
      let promiseFunc = this.saveCallbackPromises.get(date) as [((v: string[]) => void), ((e: BusinessError) => void)];
      if (promiseFunc) {
        if (err !== undefined && err.code !== 0) {
          promiseFunc[1](err);
        } else {
          promiseFunc[0](data);
        }
        this.saveCallbackPromises.delete(date);
      }
    }
  }

  actionCompleteCallback(date: number, err: BusinessError, data?: CompletedResult): void {
    if (this.completedPromises.has(date)) {
      let promiseFunc = this.completedPromises.get(date) as [
        ((v: CompletedResult) => void), ((e: BusinessError) => void)];
      if (promiseFunc) {
        if (data !== undefined && (err === undefined || err.code === 0)) {
          promiseFunc[0](data!);
        } else {
          promiseFunc[1](err);
        }
        this.completedPromises.delete(date);
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
  public pickerIndex?: int;
  public preselectedInfos?: PreselectedInfo[];
  public badgeConfig?: BadgeConfig;
  public isSlidingSupported?: boolean;
  public edgeEffect?: EdgeEffect;
  public appAlbumFilters?: string[];
  public backgroundOpacity?: double;
  public contextRecoveryInfo?: photoAccessHelper.ContextRecoveryInfo;
}

export class BaseItemInfo {
  public uri?: string;
  public mimeType?: string;
  public width?: int;
  public height?: int;
  public size?: int;
  public duration?: int;
  public photoSubType?: photoAccessHelper.PhotoSubtype;
  public dynamicRangeType?: photoAccessHelper.DynamicRangeType;
  public orientation?: int;
  public movingPhotoBadgeState?: photoAccessHelper.MovingPhotoBadgeStateType;
  public videoMode?: photoAccessHelper.VideoMode;
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

export class BrowserProps {
  public isPhotoBrowserShow: boolean = false;
  public isMovingPhotoBadgeShownValid: boolean = false;
}

export class PreselectedInfo {
  public uri: string = '';
  public preselectablePickerIndex?: int;
}

export class BadgeConfig {
  public badgeType?: BadgeType;
  public uris?: string[];
}

export class ClickResult {
  public uri: string = '';
  public isSelected: boolean = false;
}

export class PickerError {
  public functionName: string = '';
  public errorCode: int = 0;
  public message: string = '';
}

export class CompletedResult {
  public photoUris: string[] = [];
  public contextRecoveryInfo?: photoAccessHelper.ContextRecoveryInfo;
  public movingPhotoBadgeStates: photoAccessHelper.MovingPhotoBadgeStateType[] = [];
}

export class UpdatablePickerConfigs {
  public mimeType?: photoAccessHelper.PhotoViewMIMETypes;
  public mimeTypeFilter?: photoAccessHelper.MimeTypeFilter;
  public maxSelectNumber?: int;
  public maxPhotoSelectNumber?: int;
  public maxVideoSelectNumber?: int;
  public selectMode?: SelectMode;
  public singleSelectionMode?: photoAccessHelper.SingleSelectionMode;
  public isRepeatSelectSupported?: boolean;
  public preselectedUris?: string[];
  public checkBoxColor?: string;
  public checkboxTextColor?: string;
  public backgroundColor?: string;
  public photoBrowserBackgroundColorMode?: PickerColorMode;
  public uiComponentColorMode?: PickerColorMode;
  public isSlidingSupported?: boolean;
  public edgeEffect?: EdgeEffect;
  public autoPlayScenes?: photoAccessHelper.AutoPlayScene[];
  public appAlbumFilters?: string[];
  public backgroundOpacity?: double;
  public photoBrowserMargin?: Margin;
  public gridMargin?: Margin;
}

export enum DataType {
  SET_SELECTED_URIS = 1,
  SET_ALBUM_URI = 2,
  SET_SELECTED_INFO = 3,
  SET_BADGE_CONFIGS = 4,
  SET_ITEM_CLICK_RESULT = 5
}

export enum BadgeType {
  BADGE_UPLOADED = 0
}

enum BadgeOptionType {
  SET_DATA = 1,
  ADD_DATA = 2,
  DELETE_DATA = 3
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

export type SelectCallback = (uri: string) => void;
export type DeSelectCallback = (uri: string) => void;
export type ItemClickedCallback = (itemInfo: ItemInfo, clickType: ClickType) => boolean;
export type EnterPhotoBrowserCallback = (photoBrowserInfo: PhotoBrowserInfo) => boolean;
export type ExitPhotoBrowserCallback = (photoBrowserInfo: PhotoBrowserInfo) => boolean;
export type PickerControllerReadyCallback = () => void;
export type PhotoBrowserChangedCallback = (browserItemInfo: BaseItemInfo) => boolean;
export type MovingPhotoBadgeStateChangedCallback =
  (uri: string, state: photoAccessHelper.MovingPhotoBadgeStateType) => void;
export type ScrollStopAtStartCallback = () => void;
export type ScrollStopAtEndCallback = () => void;
export type ItemClickedNotifyCallback = (itemInfo: ItemInfo, clickType: ClickType) => void;
export type PinchGridSwitchedCallback = (gridLevel: photoAccessHelper.GridLevel) => void;
export type PhotoBrowserChangeStartCallback = (targetPhotoInfo: BaseItemInfo) => void;
export type ErrorCallback = (pickerError: PickerError) => void;
export type PhotoBrowserZoomCallback = (scale: double) => void;

@Component
export struct PhotoPickerComponent {
  pickerOptions?: PickerOptions | undefined;
  onSelect?: SelectCallback;
  onDeselect?: DeSelectCallback;
  onItemClicked?: ItemClickedCallback;
  onEnterPhotoBrowser?: EnterPhotoBrowserCallback;
  onExitPhotoBrowser?: ExitPhotoBrowserCallback;
  onPickerControllerReady?: PickerControllerReadyCallback;
  onPhotoBrowserChanged?: PhotoBrowserChangedCallback;
  onSelectedItemsDeleted?: ItemsDeletedCallback;
  onExceedMaxSelected?: ExceedMaxSelectedCallback;
  onCurrentAlbumDeleted?: CurrentAlbumDeletedCallback;
  onVideoPlayStateChanged?: videoPlayStateChangedCallback;
  onMovingPhotoBadgeStateChanged?: MovingPhotoBadgeStateChangedCallback;
  onPhotoBrowserChangeStart?: PhotoBrowserChangeStartCallback;
  onItemClickedNotify?: ItemClickedNotifyCallback;
  onScrollStopAtStart?: ScrollStopAtStartCallback;
  onScrollStopAtEnd?: ScrollStopAtEndCallback;
  onPinchGridSwitched?: PinchGridSwitchedCallback;
  onError?: ErrorCallback;
  onPhotoBrowserZoom?: PhotoBrowserZoomCallback;
  @ObjectLink @Watch('onChanged') pickerController: PickerController = new PickerController();
  private proxy: SecurityUIExtensionProxy | undefined;
  @State revokeIndex: int = 0;
  private badgeConfigIsSending: boolean = false;
  private isPhotoBrowserShow: boolean = false;
  private isMovingPhotoBadgeShownValid: boolean = false;
  private badgeConfig?: BadgeConfig;
  private preselectedInfos?: PreselectedInfo[];

  aboutToAppear(): void {
    this.pickerController.browserPropsRegistry = (): BrowserProps => {
      let props: BrowserProps = new BrowserProps();
      props.isPhotoBrowserShow = this.isPhotoBrowserShow;
      props.isMovingPhotoBadgeShownValid = this.isMovingPhotoBadgeShownValid;
      return props;
    };
    this.badgeConfig = this.pickerOptions?.badgeConfig;
    this.preselectedInfos = this.pickerOptions?.preselectedInfos;
  }

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
    } else if (data?.has('SET_SELECTED_INFO')) {
      this.onChangeSelectedInfo(data);
    } else if (data?.has('SET_MOVINGPHOTO_STATE')) {
      this.proxy?.send({ 'movingPhotoState': data?.get('SET_MOVINGPHOTO_STATE') } as Record<string, RecordData>);
      console.info('PhotoPickerComponent onChanged: SET_MOVINGPHOTO_STATE');
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
    } else if (data?.has('SET_BADGE_CONFIGS')) {
      this.onChangedBadgeConfigs(data);
    } else if (data?.has('UPDATE_CONFIG')) {
      this.onUpdateConfig(data);
    } else if (data?.has('SET_ITEM_CLICK_RESULT')) {
      this.onChangeItemClickResult(data);
    } else if (data?.has('COMPLETED')) {
      this.onComplete(data);
    } else {
      console.info('PhotoPickerComponent onChanged: other case');
    }
  }

  private onChangeSelectedInfo(data?: Map<string, Object>): void {
    let preselectedInfos: PreselectedInfo[] = data?.get('SET_SELECTED_INFO') as PreselectedInfo[];
    if (!preselectedInfos) {
      return;
    }
    this.preselectedInfos = preselectedInfos;
    let firstBatch: PreselectedInfo[] = preselectedInfos.slice(0, BATCH_PRESELECTED_INFOS);
    this.proxy?.send({
      'uriAndPickerIndexLists': firstBatch,
      'index': 0,
      'preselectedInfosIsOver': preselectedInfos.length <= BATCH_PRESELECTED_INFOS
    } as Record<string, RecordData>);
    console.info('PhotoPickerComponent onChanged: SET_SELECTED_INFO ' + firstBatch.length);
  }

  private onChangedBadgeConfigs(data?: Map<string, Object>): void {
    if (this.badgeConfigIsSending) {
      return;
    }
    let badgeConfig: BadgeConfig = data?.get('SET_BADGE_CONFIGS') as BadgeConfig;
    let optionType: BadgeOptionType = data?.get('BADGE_CONFIGS_OPTION_TYPE') as BadgeOptionType;
    if (badgeConfig?.uris && badgeConfig.uris!.length > MAX_BADGE_CONFIG_SIZE) {
      badgeConfig.uris = badgeConfig.uris!.slice(0, MAX_BADGE_CONFIG_SIZE);
    }
    this.badgeConfig = badgeConfig;
    this.badgeConfigIsSending = true;
    this.proxy?.send({ 'needSendBadgeConfigs': true, 'BadgeOptionType': optionType } as Record<string, RecordData>);
    console.info('PhotoPickerComponent onChanged: SET_BADGE_CONFIGS');
  }

  private onUpdateConfig(data?: Map<string, Object>): void {
    this.proxy?.send({ 'updateConfig': data?.get('UPDATE_CONFIG') as UpdatablePickerConfigs } as Record<string, RecordData>);
    console.info('PhotoPickerComponent onChanged: UPDATE_CONFIG');
  }

  private onChangeItemClickResult(data?: Map<string, Object>): void {
    let itemClickResult: ClickResult[] = data?.get('SET_ITEM_CLICK_RESULT') as ClickResult[];
    if (itemClickResult && Array.isArray(itemClickResult) && itemClickResult.length > 0) {
      this.proxy?.send({ 'itemClickResult': itemClickResult } as Record<string, RecordData>);
      console.info('PhotoPickerComponent onChanged: SET_ITEM_CLICK_RESULT ' + itemClickResult.length);
    }
  }

  private onComplete(data?: Map<string, Object>): void {
    this.proxy?.send({
      'completed': data?.get('COMPLETED'),
      'date': data?.get('COMPLETE_DATE')
    } as Record<string, RecordData>);
    console.info('PhotoPickerComponent onChanged: COMPLETED');
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

  private getClickType(wantParam: Record<string, RecordData>): ClickType {
    let clickType: ClickType = ClickType.SELECTED;
    let type: string = wantParam['clickType'] as string;
    if (type === 'select') {
      clickType = ClickType.SELECTED;
    } else if (type === 'deselect') {
      clickType = ClickType.DESELECTED;
    } else {
      console.info('PhotoPickerComponent onReceive: other clickType');
    }
    return clickType;
  }

  private getItemInfo(wantParam: Record<string, RecordData>): ItemInfo {
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
    itemInfo.photoSubType = checkValueRetUndefined<photoAccessHelper.PhotoSubtype>(wantParam['subtype']);
    itemInfo.dynamicRangeType = checkValueRetUndefined<photoAccessHelper.DynamicRangeType>(wantParam['dynamicRangeType']);
    itemInfo.orientation = checkValueRetUndefined<int>(wantParam['imageOrientation']);
    itemInfo.movingPhotoBadgeState =
      checkValueRetUndefined<photoAccessHelper.MovingPhotoBadgeStateType>(wantParam['movingPhotoBadgeState']);
    itemInfo.videoMode = checkValueRetUndefined<photoAccessHelper.VideoMode>(wantParam['videoMode']);
    return itemInfo;
  }

  private handleItemClick(wantParam: Record<string, RecordData>): void {
    if (this.onItemClicked) {
      let clickType: ClickType = this.getClickType(wantParam);
      let itemInfo: ItemInfo = this.getItemInfo(wantParam);
      let itemType: string = wantParam['itemType'] as string;
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

  private handleItemClickedNotify(wantParam: Record<string, RecordData>): void {
    if (this.onItemClickedNotify) {
      let clickType: ClickType = this.getClickType(wantParam);
      let itemInfo: ItemInfo = this.getItemInfo(wantParam);
      this.onItemClickedNotify!(itemInfo, clickType);
      console.info('PhotoPickerComponent onReceive: onItemClickedNotify = ' + clickType);
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
    this.isPhotoBrowserShow = isEnter;
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
    } else if (dataType === 'onScrollStopAtStart') {
      if (this.onScrollStopAtStart) {
        this.onScrollStopAtStart!();
      }
    } else if (dataType === 'onScrollStopAtEnd') {
      if (this.onScrollStopAtEnd) {
        this.onScrollStopAtEnd!();
      }
    } else if (dataType === 'onPreselectedInfo') {
      this.handlePreselectedInfoSend(wantParam);
    } else {
      console.info('PhotoPickerComponent onReceive: other case');
    }
  }

  private handlePhotoBrowserChange(wantParam: Record<string, RecordData>): void {
    let browserItemInfo: BaseItemInfo = new BaseItemInfo();
    browserItemInfo.uri = wantParam['uri'] as string;
    browserItemInfo.photoSubType = checkValueRetUndefined<photoAccessHelper.PhotoSubtype>(wantParam['photoSubType']);
    browserItemInfo.mimeType = checkValueRetUndefined<string>(wantParam['mimeType']);
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

  private handleCompletedCallback(wantParam: Record<string, RecordData>): void {
    let completedResult: CompletedResult | undefined = checkValueRetUndefined<CompletedResult>(wantParam['data']);
    this.pickerController.actionCompleteCallback(wantParam['date'] as number,
      { 'name': '', 'code': wantParam['code'] as int, 'message': wantParam['message'] as string }, completedResult);
    console.info('PhotoPickerComponent onReceive: handleCompletedCallback');
  }

  private sendInitialBadgeConfigs(): void {
    if (this.badgeConfig && this.badgeConfig!.uris && this.badgeConfig!.uris!.length > 0) {
      this.badgeConfigIsSending = true;
      this.proxy?.send({
        'needSendBadgeConfigs': true, 'BadgeOptionType': BadgeOptionType.SET_DATA
      } as Record<string, RecordData>);
      console.info('PhotoPickerComponent remoteReady: send badgeConfigs signal');
    }
  }

  private handleBadgeConfigSend(wantParam: Record<string, RecordData>): void {
    let index: int = wantParam['nextIndex'] as int;
    if (!this.badgeConfig || !this.badgeConfig!.uris || this.badgeConfig!.uris!.length === 0) {
      return;
    }
    let uris: string[] = this.badgeConfig!.uris!;
    let total: number = Math.ceil(uris.length / BATCH_BADGE_CONFIG_SIZE);
    let unit: string[] = uris.slice(BATCH_BADGE_CONFIG_SIZE * index, BATCH_BADGE_CONFIG_SIZE * (index + 1));
    this.proxy?.send({
      'badgeConfig': unit,
      'index': index,
      'isOver': index >= total,
      'badgeConfigType': this.badgeConfig!.badgeType
    } as Record<string, RecordData>);
    if (index >= total) {
      this.badgeConfigIsSending = false;
    }
    console.info('PhotoPickerComponent onReceive: onBadgeConfigSend index=' + index);
  }

  private sendInitialPreselectedInfos(): void {
    if (this.preselectedInfos && this.preselectedInfos!.length > 0) {
      let firstBatch: PreselectedInfo[] = this.preselectedInfos!.slice(0, BATCH_PRESELECTED_INFOS);
      this.proxy?.send({
        'uriAndPickerIndexLists': firstBatch,
        'index': 0,
        'preselectedInfosIsOver': this.preselectedInfos!.length <= BATCH_PRESELECTED_INFOS
      } as Record<string, RecordData>);
      console.info('PhotoPickerComponent remoteReady: send preselectedInfos ' + firstBatch.length);
    }
  }

  private handlePreselectedInfoSend(wantParam: Record<string, RecordData>): void {
    if (!this.preselectedInfos || this.preselectedInfos!.length === 0) {
      return;
    }
    let nextIndex: int = wantParam['nextIndex'] as int;
    let batch: PreselectedInfo[] =
      this.preselectedInfos!.slice(nextIndex * BATCH_PRESELECTED_INFOS, (nextIndex + 1) * BATCH_PRESELECTED_INFOS);
    let total: number = Math.ceil(this.preselectedInfos!.length / BATCH_PRESELECTED_INFOS);
    this.proxy?.send({
      'uriAndPickerIndexLists': batch,
      'index': nextIndex,
      'preselectedInfosIsOver': nextIndex >= total
    } as Record<string, RecordData>);
    console.info('PhotoPickerComponent onReceive: onPreselectedInfo index=' + nextIndex);
  }

  private onMovingPhotoBadgeStateChangeSend(wantParam: Record<string, RecordData>): void {
    let uri: string = wantParam['uri'] as string;
    let state: photoAccessHelper.MovingPhotoBadgeStateType =
      wantParam['enabledMovingPhotoBadge'] as photoAccessHelper.MovingPhotoBadgeStateType;
    if (uri && this.onMovingPhotoBadgeStateChanged) {
      this.onMovingPhotoBadgeStateChanged!(uri, state);
    }
    console.info('PhotoPickerComponent onReceive: onMovingPhotoBadgeStateChange');
  }

  private handleGridLevelChanged(wantParam: Record<string, RecordData>): void {
    if (this.onPinchGridSwitched) {
      this.onPinchGridSwitched!(wantParam['gridLevel'] as photoAccessHelper.GridLevel);
    }
    console.info('PhotoPickerComponent onReceive: onGridLevelChanged');
  }

  private handlePhotoBrowserZoom(wantParam: Record<string, RecordData>): void {
    if (this.onPhotoBrowserZoom) {
      this.onPhotoBrowserZoom!(wantParam['scale'] as double);
    }
    console.info('PhotoPickerComponent onReceive: onPhotoBrowserZoom');
  }

  private handleOnPhotoBrowserChangeStart(wantParam: Record<string, RecordData>): void {
    let targetPhotoInfo: BaseItemInfo = new BaseItemInfo();
    targetPhotoInfo.uri = checkValueRetUndefined<string>(wantParam['uri']);
    targetPhotoInfo.photoSubType = checkValueRetUndefined<photoAccessHelper.PhotoSubtype>(wantParam['photoSubType']);
    targetPhotoInfo.movingPhotoBadgeState =
      checkValueRetUndefined<photoAccessHelper.MovingPhotoBadgeStateType>(wantParam['movingPhotoBadgeState']);
    if (this.onPhotoBrowserChangeStart) {
      this.onPhotoBrowserChangeStart!(targetPhotoInfo);
    }
    console.info('PhotoPickerComponent onReceive: onPhotoBrowserChangeStart = ' +
      this.pickerController.encrypt(targetPhotoInfo.uri));
  }

  private handleOnError(wantParam: Record<string, RecordData>): void {
    let pickerError: PickerError = new PickerError();
    pickerError.functionName = checkValueRetUndefined<string>(wantParam['functionName']) ?? '';
    pickerError.errorCode = checkValueRetUndefined<int>(wantParam['errorCode']) ?? 0;
    pickerError.message = checkValueRetUndefined<string>(wantParam['errorMessage']) ?? '';
    if (this.onError) {
      this.onError!(pickerError);
    }
    console.info('PhotoPickerComponent onReceive: onError = ' + pickerError.errorCode);
  }

  private handleOnReceive(wantParam: Record<string, RecordData>): void {
    let dataType: string = wantParam['dataType'] as string;
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
        this.onPickerControllerReady!();
        console.info('PhotoPickerComponent onReceive: onPickerControllerReady');
      }
      this.sendInitialBadgeConfigs();
      this.sendInitialPreselectedInfos();
    } else if (dataType === 'onPhotoBrowserChanged') {
      this.handlePhotoBrowserChange(wantParam);
    } else if (dataType === 'onPhotoBrowserChangeStart') {
      this.handleOnPhotoBrowserChangeStart(wantParam);
    } else if (dataType === 'onVideoPlayStateChanged') {
      this.handleVideoPlayStateChange(wantParam)
    } else if (dataType === 'replaceCallback') {
      this.handleReplaceCallback(wantParam);
    } else if (dataType === 'createCallback') {
      this.handleCreateCallback(wantParam);
    } else if (dataType === 'saveCallback') {
      this.handleSaveCallback(wantParam);
    } else if (dataType === 'completed') {
      this.handleCompletedCallback(wantParam);
    } else if (dataType === 'onBadgeConfigSend') {
      this.handleBadgeConfigSend(wantParam);
    } else if (dataType === 'onMovingPhotoBadgeStateChange') {
      this.onMovingPhotoBadgeStateChangeSend(wantParam);
    } else if (dataType === 'onGridLevelChanged') {
      this.handleGridLevelChanged(wantParam);
    } else if (dataType === 'onPhotoBrowserZoom') {
      this.handlePhotoBrowserZoom(wantParam);
    } else if (dataType === 'onError') {
      this.handleOnError(wantParam);
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

  private parseAppAlbumFilters(appAlbumFilters?: string[]): string[] | undefined {
    if (!appAlbumFilters) {
      return undefined;
    }
    if (appAlbumFilters.length > 3) {
      return appAlbumFilters.slice(0, 3);
    }
    return appAlbumFilters;
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
            'pickerIndex': this.pickerOptions?.pickerIndex,
            'isSlidingSupported': this.pickerOptions?.isSlidingSupported,
            'edgeEffect': this.pickerOptions?.edgeEffect,
            'appAlbumFilters': this.parseAppAlbumFilters(this.pickerOptions?.appAlbumFilters),
            'backgroundOpacity': this.pickerOptions?.backgroundOpacity,
            'contextRecoveryInfo': this.pickerOptions?.contextRecoveryInfo,
            'isOnItemClickedNotifySet': this.onItemClickedNotify ? true : false,
            'isOnScrollStopAtStartSet': this.onScrollStopAtStart ? true : false,
            'isOnScrollStopAtEndSet': this.onScrollStopAtEnd ? true : false,
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