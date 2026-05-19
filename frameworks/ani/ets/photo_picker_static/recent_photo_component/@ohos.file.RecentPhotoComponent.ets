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

import { Component, Row, Column, Builder, SecurityUIExtensionComponent } from '@ohos.arkui.component';
import photoAccessHelper from '@ohos.file.photoAccessHelper';
import { BaseItemInfo, PickerColorMode } from '@ohos.file.PhotoPickerComponent';
import { RecordData } from '@ohos.base';

const FILTER_MEDIA_TYPE_ALL = 'FILTER_MEDIA_TYPE_ALL';
const FILTER_MEDIA_TYPE_IMAGE = 'FILTER_MEDIA_TYPE_IMAGE';
const FILTER_MEDIA_TYPE_VIDEO = 'FILTER_MEDIA_TYPE_VIDEO';
const FILTER_MEDIA_TYPE_IMAGE_MOVING_PHOTO = 'FILTER_MEDIA_TYPE_IMAGE_MOVING_PHOTO';

export class RecentPhotoInfo {
  public dateTaken?: long;
  public identifier?: string;
}

export enum PhotoSource {
  ALL = 0,
  CAMERA = 1,
  SCREENSHOT = 2
}

export class RecentPhotoOptions {
  public period?: int;
  public MIMEType?: photoAccessHelper.PhotoViewMIMETypes;
  public photoSource?: PhotoSource;
  public isAutoRefreshSupported?: boolean;
  public colorMode?: PickerColorMode;
}

export type RecentPhotoCheckResultCallback = (recentPhotoExists: boolean) => void;

export type RecentPhotoClickCallback = (recentPhotoInfo: BaseItemInfo) => boolean;

export type RecentPhotoCheckInfoCallback = (recentPhotoExists: boolean, info: RecentPhotoInfo) => void;

function checkValueRetUndefined<T>(value: Object | undefined | null): T | undefined {
  return (value !== undefined && value !== null) ? (value as T) : undefined;
}

@Component
export struct RecentPhotoComponent {
  public recentPhotoOptions: RecentPhotoOptions | undefined;
  public onRecentPhotoCheckResult?: RecentPhotoCheckResultCallback;
  public onRecentPhotoClick?: RecentPhotoClickCallback;
  public onRecentPhotoCheckInfo?: RecentPhotoCheckInfoCallback;

  private handleOnReceive(wantParam: Record<string, RecordData>): void {
    console.info('RecentPhotoComponent OnReceive:' + this.encrypt(JSON.stringify(wantParam)));
    let dataType: string = wantParam['dataType'] as string;
    if (dataType === 'checkResult') {
      if (this.onRecentPhotoCheckResult) {
        this.onRecentPhotoCheckResult!(wantParam['isExist'] as boolean);
      }
    } else if (dataType === 'select') {
      if (this.onRecentPhotoClick) {
        let baseItemInfo: BaseItemInfo = new BaseItemInfo();
        baseItemInfo.uri = checkValueRetUndefined<string>(wantParam['uri']);
        baseItemInfo.mimeType = checkValueRetUndefined<string>(wantParam['mimeType']);
        baseItemInfo.width = checkValueRetUndefined<int>(wantParam['width']);
        baseItemInfo.height = checkValueRetUndefined<int>(wantParam['height']);
        baseItemInfo.size = checkValueRetUndefined<int>(wantParam['size']);
        baseItemInfo.duration = checkValueRetUndefined<int>(wantParam['duration']);
        this.onRecentPhotoClick!(baseItemInfo);
      } else {
        console.warn('RecentPhotoComponent onReceive data type is invalid.');
      }
    } else if (dataType === 'checkInfo') {
      if (this.onRecentPhotoCheckInfo) {
        let info: RecentPhotoInfo = new RecentPhotoInfo();
        info.identifier = checkValueRetUndefined<string>(wantParam['identifier']);
        if (wantParam['dateTaken'] != undefined) {
          if (wantParam['dateTaken'] == 0) {
            info.dateTaken = 0;
          } else {
            info.dateTaken = (wantParam['dateTaken'] as number).toLong();
          }
        } else {
          info.dateTaken = undefined;
        }
        this.onRecentPhotoCheckInfo!(wantParam['isExist'] as boolean, info);
      }
    }
  }

  private convertMIMETypeToFilterType(mimeType: photoAccessHelper.PhotoViewMIMETypes | undefined): string {
    let filterType: string = '';
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
    let regexp: RegExp = new RegExp('(\\/\\w+)\\.', 'g');
    return data.replace(regexp, '/******.');
  }

  @Builder
  build(): void {
    Row() {
      Column() {
        SecurityUIExtensionComponent({
          bundleName: 'com.ohos.photos',
          abilityName: 'RecentUIExtensionAbility',
          parameters: {
            'ability.want.params.uiExtensionType': 'recentPhoto',
            'filterMediaType': this.convertMIMETypeToFilterType(this.recentPhotoOptions?.MIMEType),
            'period': this.recentPhotoOptions?.period,
            'photoSource': this.recentPhotoOptions?.photoSource,
            'isAutoRefreshSupported': this.recentPhotoOptions?.isAutoRefreshSupported,
            'colorMode': this.recentPhotoOptions?.colorMode,
            'isFromPickerView': true,
            'isRecentPhotoCheckResultSet': this.onRecentPhotoCheckResult ? true : false
          } as Record<string, RecordData>
        })
          .height('100%')
          .width('100%')
          .onRemoteReady(() => {
            console.info('RecentPhotoComponent onRemoteReady');
          })
          .onReceive((data: Record<string, RecordData>) => {
            this.handleOnReceive(data);
          })
          .onError(() => {
            console.info('RecentPhotoComponent onError');
          })
      }
      .width('100%')
    }
    .height('100%')
  }

}

