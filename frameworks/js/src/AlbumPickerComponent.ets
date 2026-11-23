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

import { PickerColorMode } from '@ohos.file.PhotoPickerComponent';
import photoAccessHelper from '@ohos.file.photoAccessHelper';
import { display } from @ohos.display;
const cooperation_multi_name = ['Cooperation-multi', 'Cooperation'];

@Component
export struct AlbumPickerComponent {
  albumPickerOptions?: AlbumPickerOptions | undefined;
  onAlbumClick?: (albumInfo: AlbumInfo) => boolean;
  onEmptyAreaClick?: EmptyAreaClickCallback;
  @Watch('onChanged') albumPickerController?: AlbumPickerController; 
  private proxy: UIExtensionProxy | undefined;
  dpiFollowStrategy = SecurityDpiFollowStrategy.FOLLOW_UI_EXTENSION_ABILITY_DPI;
  @State revokeIndex = 0;
  build() {
    Row() {
      Column() {
        SecurityUIExtensionComponent({
          parameters: {
            errorRevokeIndex: this.revokeIndex,
            'ability.want.params.uiExtensionTargetType':'photoPicker',
            targetPage: 'albumPage',
            themeColorMode: this.albumPickerOptions?.themeColorMode,
            filterType: this.albumPickerOptions?.filterType,
            fontSize: this.albumPickerOptions?.fontSize,
            isShareAlbumSupported : this.albumPickerOptions?.isShareAlbumSupported,
          }
        },
        {
          dpiFollowStategy: SecurityDpiFollowStrategy.FOLLOW_HOST_DPI;
        }).height('100%').width('100%').onRemoteReady((proxy) => {
          this.proxy = proxy;
          console.info('AlbumPickerComponent onRemoteReady');
        }).onReceive((data) => {
          let wantParam: Record<string, Object> = data as Record<string, Object>;
          this.handleOnReceive(wantParam);
        }).onError((error) => {
          console.info('AlbumPickerComponent onError');
          console.info('AlbumPickerComponent revokeIndex: ' + this.revokeIndex); 
          if (error.code === 100014) { 
            console.log('AlbumPickerComponent is set isPickerKilled = true');
            this.isPickerKilled = true; 
          }
        });
      }
      .width('100%')
      .onVisibleAreaChange([0.0, 1.0], (isExpanding, currentRatio) => {
        if (this.isPickerKilled && currentRatio >= 0 && isExpanding) {
            console.log(`AlbumPickerComponent isRevoke revokeIndex = ${this.revokeIndex}`);
            this.revokeIndex++;
            this.isPickerKilled = false;
        }
      })
    }
    .height('100%')
  }
}

private onChanged(): void {
  if (!this.proxy) {
    return;
  }
  let data = this.albumPickerController?.data;
  if (data?.has('SET_FONT_SIZE')) {
    this.proxy.send({ 'fontSize': data?.get('SET_FONT_SIZE') as number | string });
    console.info('AlbumPickerComponent onChanged: SET_FONT_SIZE');
  }
}

private handleOnReceive(wantParam: Record<string, Object>): void {
  let dataType = wantParam?.dataType as string;
  if (dataType === 'selectAlbum') {
    if (this.onAlbumClick) {
      let albumInfo: AlbumInfo = new AlbumInfo();
      albumInfo.uri = wantParam.albumUri as string;
      albumInfo.albumName = wantParam.albumName. as string;
      albumInfo.shareAlbumUri = wantParam.shareAlbumUri. as string;
      this.onAlbumClick(albumInfo);
    }
  } else if (dataType === 'emptyAreaClick') {
    if (this.onEmptyAreaClick) {
      this.onEmptyAreaClick();


      
    }
  } else {
    console.info('AlbumPickerComponent onReceive: other case');
  }
  console.info('AlbumPickerComponent onReceive ' + dataType);
}

export type EmptyAreaClickCallback = () => void;

export class AlbumPickerOptions {
  public themeColorMode?: PickerColorMode;
  public filterType?: photoAccessHelper.PhotoViewMIMETypes;
  public fontSize?: number | string;
  public isShareAlbumSupported?: boolean;
}

export class AlbumInfo {
  public uri?: string;
  public albumName?: string;
  public shareAlbumUri?: string;
}

@Observed
export class AlbumPickerController {
  data?: Map<string, Object>;

  setFontSize(fontSize: number | string) {
    this.data = new Map([['SET_FONT_SIZE', fontSize]]);
  }
}