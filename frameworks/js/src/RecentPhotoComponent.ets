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
import { BaseItemInfo, PickerColorMode, PickerError, ErrorCallback } from '@ohos.file.PhotoPickerComponent';

const FILTER_MEDIA_TYPE_ALL = 'FILTER_MEDIA_TYPE_ALL';
const FILTER_MEDIA_TYPE_IMAGE = 'FILTER_MEDIA_TYPE_IMAGE';
const FILTER_MEDIA_TYPE_VIDEO = 'FILTER_MEDIA_TYPE_VIDEO';
const FILTER_MEDIA_TYPE_IMAGE_MOVING_PHOTO = 'FILTER_MEDIA_TYPE_IMAGE_MOVING_PHOTO';
const SECONDS_OF_ONE_DAY: number = 24 * 60 * 60;
const RECENT_PHOTO_PERIOD_INT_MAX: number = 2147483647;

@Component
export struct RecentPhotoComponent {
  public recentPhotoOptions: RecentPhotoOptions | undefined;
  public onRecentPhotoCheckResult?: RecentPhotoCheckResultCallback;
  public onRecentPhotoClick?: RecentPhotoClickCallback;
  public onRecentPhotoCheckInfo?: RecentPhotoCheckInfoCallback;
  public onError?: ErrorCallback;
  private isAssetFilterValid: boolean = true;

  aboutToAppear(): void {
    this.isAssetFilterValid = this.validateAssetFilter();
  }

  private validateAssetFilter(): boolean {
      const assetFilter = this.recentPhotoOptions?.assetFilter;
      if (assetFilter && (!Array.isArray(assetFilter) || this.checkAssetFilterInvalid(assetFilter))) {
        console.error('RecentPhotoComponent, assetFilter has value but invalid');
        if (this.onError) {
          let e = new PickerError();
          e.functionName = 'recentPhotoOptions.assetFilter';
          e.errorCode = 401;
          e.message = 'input assetFilter invalid';
          this.onError(e);
        }
        return false;
      }
      return true;
  }


  private checkAssetFilterInvalid(assetFilter: photoAccessHelper.OperationItem[]): boolean {
    // 获取所有有效的值
    const validOperationTypes = Object.values(photoAccessHelper.OperationType);
    const validPhotoKeys = Object.values(photoAccessHelper.PhotoKeys);

    //遍历数组中的每个OperationItem
    for (const item of assetFilter) {
      //检查operationType是否有值且在枚举中
      if (!item || !item.operationType || !validOperationTypes.includes(item?.operationType)) {
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
    if (this.isAssetFilterValid) {
      Row() {
        Column() {
          SecurityUIExtensionComponent({
            bundleName: 'com.ohos.photos',
            abilityName: 'RecentUIExtensionAbility',
            parameters: {
              'ability.want.params.uiExtensionType': 'recentPhoto',
              filterMediaType: this.convertMIMETypeToFilterType(this.recentPhotoOptions?.MIMEType),
            period: this.normalizePeriod(this.recentPhotoOptions?.period),
              photoSource: this.recentPhotoOptions?.photoSource as PhotoSource,
              isAutoRefreshSupported: this.recentPhotoOptions?.isAutoRefreshSupported as boolean,
              colorMode: this.recentPhotoOptions?.colorMode as PickerColorMode,
              isFromPickerView: true,
              isRecentPhotoCheckResultSet: this.onRecentPhotoCheckResult ? true : false,
              isPersistAfterClick: this.recentPhotoOptions?.isPersistAfterClick as boolean,
              isShowBadge: this.recentPhotoOptions?.isShowBadge as boolean,
              assetFilter: this.recentPhotoOptions?.assetFilter,
            }
          })
              .height('100%')
              .width('100%')
              .onRemoteReady(() => {
                console.info('RecentPhotoComponent onRemoteReady');
              })
              .onReceive((data) => {
                let wantParam: Record<string, Object> = data as Record<string, Object>;
                this.handleOnReceive(wantParam);
              })
              .onError((error) => {
                console.info('RecentPhotoComponent onError: ' + JSON.stringify(error));
                if (this.onError) {
                  let e = new PickerError();
                  e.functionName = error?.name || 'extension_exit_abnormally';
                  e.errorCode = error?.code ?? 0;
                  e.message = error?.message || 'the extension ability exited abnormally, please check the log.';
                  this.onError(e);
                }
              })
        }
      .width('100%')
      }
    .height('100%')
    }
  }

  private handleOnReceive(wantParam: Record<string, Object>): void {
    console.info('RecentPhotoComponent OnReceive:' + this.encrypt(JSON.stringify(wantParam)));
    let dataType: string = wantParam['dataType'] as string;
    if (dataType === 'checkResult') {
      if (this.onRecentPhotoCheckResult) {
        this.onRecentPhotoCheckResult(wantParam['isExist'] as boolean);
      }
    } else if (dataType === 'select') {
      if (this.onRecentPhotoClick) {
        let baseItemInfo: BaseItemInfo = new BaseItemInfo();
        baseItemInfo.uri = wantParam['uri'] as string;
        baseItemInfo.mimeType = wantParam['mimeType'] as string;
        baseItemInfo.width = wantParam['width'] as number;
        baseItemInfo.height = wantParam['height'] as number;
        baseItemInfo.size = wantParam['size'] as number;
        baseItemInfo.duration = wantParam['duration'] as number;
        this.onRecentPhotoClick(baseItemInfo);
      } else {
        console.warn('RecentPhotoComponent onReceive data type is invalid.');
      }
    } else if (dataType === 'checkInfo') {
      if (this.onRecentPhotoCheckInfo) {
        let info: RecentPhotoInfo = new RecentPhotoInfo();
        info.identifier = wantParam.identifier as string;
        info.dateTaken = wantParam.dateTaken as number;
        this.onRecentPhotoCheckInfo(wantParam.isExist as boolean, info);
      }
    }
  }

  private normalizePeriod(period: number | undefined): number {
    if (period === Number.POSITIVE_INFINITY) {
      return RECENT_PHOTO_PERIOD_INT_MAX;
    }
    if (period === undefined || Number.isNaN(period) || period <= 0 || period > RECENT_PHOTO_PERIOD_INT_MAX) {
      return SECONDS_OF_ONE_DAY;
    }
    return period;
  }
  private convertMIMETypeToFilterType(mimeType: photoAccessHelper.PhotoViewMIMETypes | undefined): string {
    let filterType: string;
    if (mimeType === photoAccessHelper.PhotoViewMIMETypes.IMAGE_TYPE) {
      filterType = FILTER_MEDIA_TYPE_IMAGE;
    } else if (mimeType === photoAccessHelper.PhotoViewMIMETypes.VIDEO_TYPE) {
      filterType = FILTER_MEDIA_TYPE_VIDEO;
    } else if (mimeType === photoAccessHelper.PhotoViewMIMETypes.MOVING_PHOTO_IMAGE_TYPE) {
      filterType = FILTER_MEDIA_TYPE_IMAGE_MOVING_PHOTO;
    } else {
      filterType = FILTER_MEDIA_TYPE_ALL;
    }
    console.info('RecentPhotoComponent convertMIMETypeToFilterType: ' + JSON.stringify(filterType));
    return filterType;
  }

  private encrypt(data: string): string {
    if (!data || data?.indexOf('file:///data/storage/') !== -1) {
      return '';
    }
    let encryptedData = data.replace(/(\/[\w()%+-]+)\./g, '/******.');
    encryptedData = encryptedData.replace(/"userComment":"([^"]|\")*"/g, '"userComment":"******"');
    encryptedData = encryptedData.replace(/"albumName":"([^"]|\")*"/g, '"albumName":"******"');
    encryptedData = encryptedData.replace(/"displayName":"([^"]|\")*"/g, '"displayName":"******"');
    return encryptedData;
  }
}

export type RecentPhotoCheckResultCallback = (recentPhotoExists: boolean) => void;

export type RecentPhotoClickCallback = (recentPhotoInfo: BaseItemInfo) => boolean;

export type RecentPhotoCheckInfoCallback = (recentPhotoExists: boolean, info: RecentPhotoInfo) => void;

export class RecentPhotoOptions {
  public period?: number;
  public MIMEType?: photoAccessHelper.PhotoViewMIMETypes;
  public photoSource?: PhotoSource;
  public isAutoRefreshSupported?: boolean;
  public colorMode?: PickerColorMode;
  public isPersistAfterClick?: boolean;
  public isShowBadge?: boolean;
  public assetFilter?: Array<photoAccessHelper.OperationItem>;
}

export class RecentPhotoInfo {
  dateTaken?: number;
  identifier?: string;
}

export enum PhotoSource {
  ALL = 0,
  CAMERA = 1,
  SCREENSHOT = 2
}
