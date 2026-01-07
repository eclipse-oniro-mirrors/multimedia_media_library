/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

import { AsyncCallback, Callback, BusinessError } from '@ohos.base';
import Context from 'application.Context';
import image from '@ohos.multimedia.image';
import dataSharePredicates from '@ohos.data.dataSharePredicates';

type RejectString = (e: BusinessError<string>) => void;

function businessError<T = void>(c: int, d?: string): BusinessError<T> {
  let err: BusinessError<T> = new BusinessError<T>();
  err.code = c;
  err.message = d ? d : "";
  return err;
}

function isNullish(d: Any): boolean {
  return (d === null || d === undefined) ? true : false;
}

class MediaLibraryAniError extends BusinessError {
  constructor(code: int, message: string) {
    super();
    this.code = code;
    this.message = message;
  }
}

export namespace userFileManager {
  loadLibrary("userfile_manager_ani");
  export native function getUserFileMgr(context: Context): UserFileManager;

  export enum FileType {
    IMAGE = 1,
    VIDEO,
    AUDIO
  }

  enum PhotoSubType {
    DEFAULT,
    SCREENSHOT,
    CAMERA
  }

  enum PositionType {
    LOCAL = 1,
    CLOUD,
    BOTH
  }

  export type ChangeEvent =
    'deviceChange'
    | 'albumChange'
    | 'imageChange'
    | 'audioChange'
    | 'videoChange'
    | 'remoteFileChange';

  interface FileAsset {
  }

  class FileAssetHandle implements FileAsset {
    private nativeHandle: long = 0;

    constructor(nativePtr: long) {
      if (this.nativeHandle == 0) {
        this.nativeHandle = nativePtr;
      }
    }
  }

  export enum AudioKey {
    URI,
    DISPLAY_NAME,
    DATE_ADDED,
    DATE_MODIFIED,
    TITLE,
    ARTIST,
    AUDIOALBUM,
    DURATION,
    FAVORITE
  }

  export enum ImageVideoKey {
    URI,
    FILE_TYPE,
    DISPLAY_NAME,
    DATE_ADDED,
    DATE_MODIFIED,
    TITLE,
    DURATION,
    WIDTH,
    HEIGHT,
    DATE_TAKEN,
    ORIENTATION,
    FAVORITE,
    POSITION,
    DATE_TRASHED,
    HIDDEN,
    USER_COMMENT,
    CAMERA_SHOT_KEY
  }

  export enum AlbumKey {
    URI,
    FILE_TYPE,
    ALBUM_NAME,
    DATE_ADDED,
    DATE_MODIFIED
  }

  export interface FetchOptions {
    fetchColumns: Array<string>;
    predicates: dataSharePredicates.DataSharePredicates;
  }

  export interface FetchResult<T> {
  }

  class FetchResultHandle<T> implements FetchResult<T> {
    private nativeHandle: long = 0;

    constructor(nativePtr: long) {
        if (this.nativeHandle == 0) {
            this.nativeHandle = nativePtr;
        }
    }
  }

  enum AlbumType {
    USER = 0,
    SYSTEM = 1024
  }

  export enum AlbumSubType {
    USER_GENERIC = 1,
    FAVORITE = 1025,
    VIDEO,
    HIDDEN,
    TRASH,
    SCREENSHOT,
    CAMERA,
    ANY = 2147483647
  }

  export interface UserFileManager {
    getPhotoAssets(options: FetchOptions, callback: AsyncCallback<FetchResult<FileAsset> | undefined>): void;
    getPhotoAssets(options: FetchOptions): Promise<FetchResult<FileAsset>>;
    release(callback: AsyncCallback<void>): void;
    release(): Promise<void>;
  }

  class UserFileManagerHandle implements UserFileManager {
    private nativeHandle: long = 0;

    constructor(nativePtr: long) {
      if (this.nativeHandle == 0) {
        this.nativeHandle = nativePtr;
      }
    }

    getPhotoAssets(options: FetchOptions, callback: AsyncCallback<FetchResult<FileAsset> | undefined>): void {
      let cb = (): MediaLibraryAniError | FetchResult<FileAsset> => {
        try {
          return this.getPhotoAssetsSync(options);
        } catch(e) {
          return e as MediaLibraryAniError;
        }
      };
      taskpool.execute(cb).then((ret: Any): void => {
        if (ret instanceof MediaLibraryAniError) {
          let err = ret as MediaLibraryAniError;
          callback(businessError(err.code as int, err.message), undefined);
        } else {
          if (isNullish(ret)) {
            callback(businessError(-1, "Operation failed"), undefined);
          } else {
            callback(businessError(0), ret as FetchResult<FileAsset>);
          }
        }
      });
    }

    getPhotoAssets(options: FetchOptions): Promise<FetchResult<FileAsset>> {
      return new Promise<FetchResult<FileAsset>>((resolve: (v: FetchResult<FileAsset>) => void,
        reject: RejectString) => {
        let cb = (): FetchResult<FileAsset> | MediaLibraryAniError => {
          try {
            return this.getPhotoAssetsSync(options);
          } catch(e) {
            return e as MediaLibraryAniError;
          }
        };
        taskpool.execute(cb).then((ret: Any): void => {
          if (ret instanceof MediaLibraryAniError) {
            let err = ret as MediaLibraryAniError;
            reject(businessError<string>(err.code as int, err.message));
          } else {
            if (isNullish(ret)) {
              reject(businessError<string>(-1, "Operation failed"));
            } else {
              resolve(ret as FetchResult<FileAsset>);
            }
          }
        });
      });
    }

    release(callback: AsyncCallback<void>): void {
      let cb = (): MediaLibraryAniError | undefined => {
        try {
          this.releaseSync();
          return undefined;
        } catch(e) {
          return e as MediaLibraryAniError;
        }
      };
      taskpool.execute(cb).then((ret: Any): void => {
        if (isNullish(ret)) {
          callback(businessError(0), undefined);
        } else {
          let err = ret as MediaLibraryAniError;
          callback(businessError(err.code as int, err.message), undefined);
        }
      });
    }

    release(): Promise<void> {
      return new Promise<void>((resolve: (v: undefined) => void, reject: (e: BusinessError<void>) => void): void => {
        let cb = (): MediaLibraryAniError | undefined => {
          try {
            this.releaseSync();
            return undefined;
          } catch(e) {
            return e as MediaLibraryAniError;
          }
        };
        taskpool.execute(cb).then((ret: Any): void => {
          if (isNullish(ret)) {
            resolve(undefined);
          } else {
            let err = ret as MediaLibraryAniError;
            reject(businessError(err.code as int, err.message));
          }
        });
      });
    }

    private native getPhotoAssetsSync(options: FetchOptions): FetchResult<FileAsset>;
    private native releaseSync(): void;
  }

  enum NotifyType {
    NOTIFY_ADD,
    NOTIFY_UPDATE,
    NOTIFY_REMOVE,
    NOTIFY_ALBUM_ADD_ASSET,
    NOTIFY_ALBUM_REMOVE_ASSET
  }

  enum DefaultChangeUri {
    DEFAULT_PHOTO_URI,
    DEFAULT_ALBUM_URI,
    DEFAULT_AUDIO_URI
  }

  export enum PrivateAlbumType {
    TYPE_FAVORITE,
    TYPE_TRASH
  }
}
