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

import { Component, Row, Column, Builder, SecurityUIExtensionComponent, SecurityUIExtensionProxy } from '@ohos.arkui.component';
import { PickerColorMode } from '@ohos.file.PhotoPickerComponent';
import photoAccessHelper from '@ohos.file.photoAccessHelper';
import { RecordData } from '@ohos.base';
import { Observed, Watch } from '@ohos.arkui.stateManagement';

export type EmptyAreaClickCallback = () => void;

export class AlbumPickerOptions {
  public themeColorMode?: PickerColorMode;
  public filterType?: photoAccessHelper.PhotoViewMIMETypes;
  public fontSize?: int | string;
}

export class AlbumInfo {
  public uri?: string;
  public albumName?: string;
}

function checkValueRetUndefined<T>(value: Object | undefined | null): T | undefined {
  return (value !== undefined && value !== null) ? (value as T) : undefined;
}

export class AlbumPickerController {
  public data?: Map<string, Object>;

  public setFontSize(fontSize: int | string): void {
    this.data = new Map<string, Object>([
      ['SET_FONT_SIZE', fontSize]
    ])
  }
}

@Component
export struct AlbumPickerComponent {
  albumPickerOptions?: AlbumPickerOptions | undefined;
  onAlbumClick?: (albumInfo: AlbumInfo) => boolean;
  onEmptyAreaClick?: EmptyAreaClickCallback;
  @Watch('onChanged') albumPickerController?: AlbumPickerController; 
  private proxy: SecurityUIExtensionProxy | undefined;

  private onChanged(): void {
    if (!this.proxy) {
      return;
    }
    let data: Map<string, Object> | undefined = this.albumPickerController?.data;
    if (data?.has('SET_FONT_SIZE')) {
      this.proxy?.send({ 'fontSize': data?.get('SET_FONT_SIZE') as number | string });
      console.info('AlbumPickerComponent onChanged: SET_FONT_SIZE');
    }
  }

  private handleOnReceive(wantParam: Record<string, RecordData>): void {
    console.info('AlbumPickerComponent handleOnReceive: ' + JSON.stringify(wantParam));
    let dataType: string = (wantParam['dataType'] as string) ?? '';
    if (dataType === 'selectAlbum') {
      if (this.onAlbumClick) {
        let albumInfo: AlbumInfo = new AlbumInfo();
        albumInfo.uri = checkValueRetUndefined<string>(wantParam['albumUri']);
        albumInfo.albumName = checkValueRetUndefined<string>(wantParam['albumName']);
        this.onAlbumClick!(albumInfo!);
      }
    } else if (dataType === 'emptyAreaClick') {
      if (this.onEmptyAreaClick) {
        this.onEmptyAreaClick!();
      }
    } else {
      console.info('AlbumPickerComponent onReceive: other case');
    }
    console.info('AlbumPickerComponent onReceive ' + dataType);
  }

  @Builder
  build(): void {
    Row() {
      Column() {
        SecurityUIExtensionComponent({
          parameters: {
            'ability.want.params.uiExtensionTargetType': 'photoPicker',
            'targetPage': 'albumPage',
            'themeColorMode': this.albumPickerOptions?.themeColorMode,
            'filterType': this.albumPickerOptions?.filterType,
            'fontSize': this.albumPickerOptions?.fontSize,
          } as Record<string, RecordData>
        })
        .height('100%')
        .width('100%')
        .onRemoteReady((proxy: SecurityUIExtensionProxy) => {
          console.info('AlbumPickerComponent onRemoteReady');
        })
        .onReceive((data: Record<string, RecordData>) => {
          this.handleOnReceive(data);
        })
        .onError(() => {
          console.info('AlbumPickerComponent onError');
        })
      }
      .width('100%')
    }
    .height('100%')
  }
}