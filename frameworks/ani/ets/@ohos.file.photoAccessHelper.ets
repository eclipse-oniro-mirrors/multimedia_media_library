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
import image from '@ohos.multimedia.image';
import dataSharePredicates from '@ohos.data.dataSharePredicates';
import Context from 'application.Context';
import { CustomColors } from '@ohos.arkui.theme';

class SizeImpl implements image.Size {
  height: int = 0;
  width: int = 0;
}

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
    super(code, new Error(message));
    this.code = code;
    this.message = message;
  }
}

export namespace photoAccessHelper {
  loadLibraryWithPermissionCheck("photo_access_helper_ani", "@ohos.file.photoAccessHelper");
  native function getPhotoAccessHelperInner(context: Context): PhotoAccessHelper;
  native function getPhotoAccessHelperWithUserIdInner(context: Context, userId: int): PhotoAccessHelper;

  export function getPhotoAccessHelper(context: Context): PhotoAccessHelper {
    return getPhotoAccessHelperInner(context);
  }

  export function getPhotoAccessHelper(context: Context, userId: int): PhotoAccessHelper {
    return getPhotoAccessHelperWithUserIdInner(context, userId);
  }

  export enum PhotoType {
    IMAGE = 1,
    VIDEO
  }

  export enum PhotoSubtype {
    DEFAULT = 0,
    SCREENSHOT = 1,
    MOVING_PHOTO = 3,
    BURST = 4,
    SLOW_MOTION_VIDEO = 6,
    SPATIAL_3DGS = 7,
  }

  export enum DynamicRangeType {
    SDR = 0,
    HDR = 1
  }

  enum ThumbnailVisibility {
    INVISIBLE = 0,
    VISIBLE = 1
  }

  export enum PositionType {
    LOCAL = 1,
    CLOUD = 2,
    LOCAL_AND_CLOUD = 3
  }

  export enum AnalysisType {
    ANALYSIS_AESTHETICS_SCORE = 0,
    ANALYSIS_LABEL,
    ANALYSIS_OCR,
    ANALYSIS_FACE,
    ANALYSIS_OBJECT,
    ANALYSIS_RECOMMENDATION,
    ANALYSIS_SEGMENTATION,
    ANALYSIS_COMPOSITION,
    ANALYSIS_SALIENCY,
    ANALYSIS_DETAIL_ADDRESS,
    ANALYSIS_HUMAN_FACE_TAG,
    ANALYSIS_HEAD_POSITION,
    ANALYSIS_BONE_POSE,
    ANALYSIS_VIDEO_LABEL,
    ANALYSIS_HIGHLIGHT,
    ANALYSIS_MULTI_CROP,
    ANALYSIS_SEARCH_INDEX = 16
  }

  export enum RecommendationType {
    QR_OR_BAR_CODE = 1,
    QR_CODE = 2,
    BAR_CODE = 3,
    ID_CARD = 4,
    PROFILE_PICTURE = 5,
    PASSPORT = 6,
    BANK_CARD = 7,
    DRIVER_LICENSE = 8,
    DRIVING_LICENSE = 9,
    FEATURED_SINGLE_PORTRAIT = 10,
    COLOR_STYLE_PHOTO = 12
  }

  export enum DeliveryMode {
    FAST_MODE = 0,
    HIGH_QUALITY_MODE = 1,
    BALANCE_MODE = 2
  }

  export enum CompatibleMode {
    ORIGINAL_FORMAT_MODE = 0,
    COMPATIBLE_FORMAT_MODE = 1
  }

  export interface MediaAssetProgressHandler {
    onProgress(progress: int): void;
  }

  export enum SourceMode {
    ORIGINAL_MODE = 0,
    EDITED_MODE = 1
  }

  export enum PhotoPermissionType {
    TEMPORARY_READ_IMAGEVIDEO = 0,
    PERSISTENT_READ_IMAGEVIDEO = 1
  }

  export enum HideSensitiveType {
    HIDE_LOCATION_AND_SHOOTING_PARAM = 0,
    HIDE_LOCATION_ONLY = 1,
    HIDE_SHOOTING_PARAM_ONLY = 2,
    NO_HIDE_SENSITIVE_TYPE = 3
  }

  export enum AuthorizationMode {
    SHORT_TIME_AUTHORIZATION = 0
  }

  export enum CompleteButtonText {
    TEXT_DONE = 0,
    TEXT_SEND = 1,
    TEXT_ADD = 2,
  }

  export enum WatermarkType {
    DEFAULT = 0,
    BRAND_COMMON = 1,
    COMMON = 2,
    BRAND = 3,
  }

  enum SupportedImageFormat {
    AVFILE_FORMAT_JPG = 'jpg',
  }

  export enum FilterOperator {
    EQUAL_TO = 0,
    NOT_EQUAL_TO = 1,
    MORE_THAN = 2,
    LESS_THAN = 3,
    MORE_THAN_OR_EQUAL_TO = 4,
    LESS_THAN_OR_EQUAL_TO = 5,
    BETWEEN = 6,
  }

  class MimeTypeFilter {
    mimeTypeArray: Array<string>;
  }

  class FileSizeFilter {
    filterOperator: FilterOperator;
    fileSize: long;
    extraFileSize?: long;
  }

  class VideoDurationFilter {
      filterOperator: FilterOperator;
      videoDuration: int;
      extraVideoDuration?: int;
  }

  export interface RequestOptions {
    deliveryMode: DeliveryMode;
    sourceMode?: SourceMode;
    compatibleMode?: CompatibleMode;
    mediaAssetProgressHandler?: MediaAssetProgressHandler;
  }

  export interface MediaAssetDataHandler<T> {
    onDataPrepared(data: T, map?: Map<string, string>): void;
  }

  export interface QuickImageDataHandler<T> {
    onDataPrepared(data: T, imageSource: image.ImageSource, map: Map<string, string>): void;
  }

  export interface PhotoProxy {}

  type MediaAssetDataType = image.ImageSource | ArrayBuffer | MovingPhoto | boolean;
  type QuickImageDataType = image.Picture;

  interface requestVideoFileParam {
    asset: PhotoAsset;
    requestOptions: RequestOptions;
    fileUri: string;
    dataHandler: MediaAssetDataHandler<boolean>;
  }

  interface AlbumChangeInfos {
    type: NotifyChangeType;
    albumChangeDatas: Array<AlbumChangeData>| null;
    isForRecheck: boolean;
  }

  class AlbumChangeInfosHandle implements AlbumChangeInfos {
    constructor() {}
    type: NotifyChangeType;
    albumChangeDatas: Array<AlbumChangeData>| null = null;
    isForRecheck: boolean;
  }

  interface AlbumChangeData {
    albumBeforeChange: AlbumChangeInfo | null;
    albumAfterChange: AlbumChangeInfo | null;
    version: long;
  }

  class AlbumChangeDataHandle implements AlbumChangeData {
    constructor() {}
    albumBeforeChange: AlbumChangeInfo | null = null;
    albumAfterChange: AlbumChangeInfo | null = null;
    version: long = 0;
  }

  interface AlbumChangeInfo {
    albumType: AlbumType;
    albumSubtype: AlbumSubtype;
    albumName: string;
    albumUri: string;
    imageCount: int;
    videoCount: int;
    count: int;
    coverUri: string;
    hiddenCount: int;
    hiddenCoverUri: string;
    isCoverChanged: boolean;
    isHiddenCoverChanged: boolean;
    coverInfo?: PhotoAssetChangeInfo;
    hiddenCoverInfo?: PhotoAssetChangeInfo;
  }

  class AlbumChangeInfoHandle implements AlbumChangeInfo {
    constructor() {}
    albumType: AlbumType;
    albumSubtype: AlbumSubtype;
    albumName: string = "";
    albumUri: string = "";
    imageCount: int = 0;
    videoCount: int = 0;
    count: int = 0;
    coverUri: string = "";
    hiddenCount: int = 0;
    hiddenCoverUri: string = "";
    isCoverChanged: boolean;
    isHiddenCoverChanged: boolean;
    coverInfo?: PhotoAssetChangeInfo;
    hiddenCoverInfo?: PhotoAssetChangeInfo;
  }

  interface PhotoAssetChangeInfos {
    type: NotifyChangeType;
    assetChangeDatas: Array<PhotoAssetChangeData> | null;
    isForRecheck: boolean;
  }

  class PhotoAssetChangeInfosHandle implements PhotoAssetChangeInfos {
    constructor() {}
    type: NotifyChangeType;
    assetChangeDatas: Array<PhotoAssetChangeData> | null = null;
    isForRecheck: boolean;
  }

  interface PhotoAssetChangeData {
    assetBeforeChange: PhotoAssetChangeInfo | null;
    assetAfterChange: PhotoAssetChangeInfo | null;
    isContentChanged: boolean;
    isDeleted: boolean;
    thumbnailChangeStatus: ThumbnailChangeStatus;
    version: long;
  }

  class PhotoAssetChangeDataHandle implements PhotoAssetChangeData {
    constructor() {}
    assetBeforeChange: PhotoAssetChangeInfo | null = null;
    assetAfterChange: PhotoAssetChangeInfo | null = null;
    isContentChanged: boolean;
    isDeleted: boolean;
    thumbnailChangeStatus: ThumbnailChangeStatus;
    version: long = 0;
  }

  enum ThumbnailChangeStatus {
    THUMBNAIL_NOT_EXISTS = 0,
    THUMBNAIL_ADD = 1,
    THUMBNAIL_UPDATE = 2,
    THUMBNAIL_NOT_CHANGE = 3
  }

  enum StrongAssociationType {
    NORMAL = 0,
    CLOUD_ENHANCEMENT = 1
  }

  interface PhotoAssetChangeInfo {
    uri: string;
    mediaType: PhotoType;
    albumUri: string;
    fileId: int;
    dateDay: string;
    isFavorite: boolean;
    isHidden: boolean;
    strongAssociation: StrongAssociationType;
    thumbnailVisible: ThumbnailVisibility;
    dateTrashedMs: long;
    dateAddedMs: long;
    dateTakenMs: long;
  }

  export interface AlbumOrder {
    albumId: int;
    albumOrder: int;
    orderSection: int;
    orderType: int;
    orderStatus: int;
  }

  export class MediaAssetManager {
    static onMediaAssetDataPrepared(data: MediaAssetDataType,
      dataHandler: MediaAssetDataHandler<MediaAssetDataType>, map?: Map<string, string>): void {
      if (dataHandler !== undefined && dataHandler !== null) {
        dataHandler.onDataPrepared(data, map);
      }
    }

    static onQuickImageDataPrepared(data: QuickImageDataType, imageSource: image.ImageSource,
      map: Map<string, string>, dataHandler: QuickImageDataHandler<QuickImageDataType>): void {
      if (dataHandler !== undefined && dataHandler !== null) {
        dataHandler.onDataPrepared(data, imageSource, map);
      }
    }

    static onMediaAssetProgress(progress: int, progressHandler: MediaAssetProgressHandler): void {
      if (progressHandler !== undefined && progressHandler !== null) {
        progressHandler.onProgress(progress);
      }
    }

    static requestImage(context: Context, asset: PhotoAsset, requestOptions: RequestOptions,
      dataHandler: MediaAssetDataHandler<image.ImageSource>): Promise<string> {
      return new Promise<string>((resolve: (v: string) => void, reject: RejectString) => {
        let cb = (): string | MediaLibraryAniError => {
          let result: string | MediaLibraryAniError;
          try {
            result = MediaAssetManager.requestImageInner(context, asset, requestOptions, dataHandler);
          } catch(e) {
            result = e as MediaLibraryAniError;
          }
          return result;
        };
        taskpool.execute(cb).then((ret: Any): void => {
          if (ret instanceof MediaLibraryAniError) {
            let err = ret as MediaLibraryAniError;
            reject(businessError<string>(err.code as int, err.message));
          } else {
            if (isNullish(ret)) {
              reject(businessError<string>(-1, "Operation failed"));
            } else {
              resolve(ret as string);
            }
          }
        });
      });
    }

    static quickRequestImage(context: Context, asset: PhotoAsset, requestOptions: RequestOptions,
      dataHandler: QuickImageDataHandler<image.Picture>): Promise<string> {
      return new Promise<string>((resolve: (v: string) => void, reject: RejectString) => {
        let cb = (): string | MediaLibraryAniError => {
          let result: string | MediaLibraryAniError;
          try {
            result = MediaAssetManager.quickRequestImageInner(context, asset, requestOptions, dataHandler);
          } catch(e) {
            result = e as MediaLibraryAniError;
          }
          return result;
        };
        taskpool.execute(cb).then((ret: Any): void => {
          if (ret instanceof MediaLibraryAniError) {
            let err = ret as MediaLibraryAniError;
            reject(businessError<string>(err.code as int, err.message));
          } else {
            if (isNullish(ret)) {
              reject(businessError<string>(-1, "Operation failed"));
            } else {
              resolve(ret as string);
            }
          }
        });
      });
    }

    static requestImageData(context: Context, asset: PhotoAsset, requestOptions: RequestOptions,
      dataHandler: MediaAssetDataHandler<ArrayBuffer>): Promise<string> {
      return new Promise<string>((resolve: (v: string) => void, reject: RejectString) => {
        let cb = (): string | MediaLibraryAniError => {
          let result: string | MediaLibraryAniError;
          try {
            result = MediaAssetManager.requestImageDataInner(context, asset, requestOptions, dataHandler);
          } catch(e) {
            result = e as MediaLibraryAniError;
          }
          return result;
        };
        taskpool.execute(cb).then((ret: Any): void => {
          if (ret instanceof MediaLibraryAniError) {
            let err = ret as MediaLibraryAniError;
            reject(businessError<string>(err.code as int, err.message));
          } else {
            if (isNullish(ret)) {
              reject(businessError<string>(-1, "Operation failed"));
            } else {
              resolve(ret as string);
            }
          }
        });
      });
    }

    static requestMovingPhoto(context: Context, asset: PhotoAsset, requestOptions: RequestOptions,
      dataHandler: MediaAssetDataHandler<MovingPhoto>): Promise<string> {
      return new Promise<string>((resolve: (v: string) => void, reject: RejectString) => {
        let cb = (): string | MediaLibraryAniError => {
          let result: string | MediaLibraryAniError;
          try {
            result = MediaAssetManager.requestMovingPhotoInner(context, asset, requestOptions, dataHandler);
          } catch(e) {
            result = e as MediaLibraryAniError;
          }
          return result;
        };
        taskpool.execute(cb).then((ret: Any): void => {
          if (ret instanceof MediaLibraryAniError) {
            let err = ret as MediaLibraryAniError;
            reject(businessError<string>(err.code as int, err.message));
          } else {
            if (isNullish(ret)) {
              reject(businessError<string>(-1, "Operation failed"));
            } else {
              resolve(ret as string);
            }
          }
        });
      });
    }

    static cancelRequest(context: Context, requestId: string): Promise<void> {
      return new Promise<void>((resolve: (v: undefined) => void, reject: (e: BusinessError<void>) => void): void => {
        let cb = (): MediaLibraryAniError | undefined => {
          try {
            MediaAssetManager.cancelRequestInner(context, requestId);
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

    static requestVideoFile(context: Context, asset: PhotoAsset, requestOptions: RequestOptions,
      fileUri: string, dataHandler: MediaAssetDataHandler<boolean>): Promise<string> {
      return new Promise<string>((resolve: (v: string) => void, reject: RejectString) => {
        let cb = (): string | MediaLibraryAniError => {
          let result: string | MediaLibraryAniError;
          let param: requestVideoFileParam = {
            asset: asset,
            requestOptions: requestOptions,
            fileUri: fileUri,
            dataHandler: dataHandler
          };
          try {
            result = MediaAssetManager.requestVideoFileInner(context, param);
          } catch(e) {
            result = e as MediaLibraryAniError;
          }
          return result;
        };
        taskpool.execute(cb).then((ret: Any): void => {
          if (ret instanceof MediaLibraryAniError) {
            let err = ret as MediaLibraryAniError;
            reject(businessError<string>(err.code as int, err.message));
          } else {
            if (isNullish(ret)) {
              reject(businessError<string>(-1, "Operation failed"));
            } else {
              resolve(ret as string);
            }
          }
        });
      });
    }

    static loadMovingPhoto( context: Context, imageFileUri: string, videoFileUri: string ): Promise<MovingPhoto> {
      return new Promise<MovingPhoto>((resolve: (v: MovingPhoto) => void, reject: RejectString) => {
        let cb = (): MovingPhoto | MediaLibraryAniError => {
          let result: MovingPhoto | MediaLibraryAniError;
          try {
            result = MediaAssetManager.loadMovingPhotoInner(context, imageFileUri, videoFileUri);
          } catch(e) {
            result = e as MediaLibraryAniError;
          }
          return result;
        };
        taskpool.execute(cb).then((ret: Any): void => {
          if (ret instanceof MediaLibraryAniError) {
            let err = ret as MediaLibraryAniError;
            reject(businessError<string>(err.code as int, err.message));
          } else {
            if (isNullish(ret)) {
              reject(businessError<string>(-1, "Operation failed"));
            } else {
              resolve(ret as MovingPhoto);
            }
          }
        });
      });
    }

    native static requestImageInner(context: Context, asset: PhotoAsset, requestOptions: RequestOptions,
      dataHandler: MediaAssetDataHandler<image.ImageSource>): string;
    native static quickRequestImageInner(context: Context, asset: PhotoAsset, requestOptions: RequestOptions,
      dataHandler: QuickImageDataHandler<image.Picture>): string;
    native static requestImageDataInner(context: Context, asset: PhotoAsset, requestOptions: RequestOptions,
      dataHandler: MediaAssetDataHandler<ArrayBuffer>): string;
    native static requestMovingPhotoInner(context: Context, asset: PhotoAsset, requestOptions: RequestOptions,
      dataHandler: MediaAssetDataHandler<MovingPhoto>): string;
    native static cancelRequestInner(context: Context, requestId: string): void;
    native static requestVideoFileInner(context: Context, param: requestVideoFileParam): string;
    native static loadMovingPhotoInner(context: Context, imageFileUri: string, videoFileUri: string): MovingPhoto;
  }

  export type MemberType = int | long | double | string | boolean;

  export interface PhotoAsset {
    readonly uri: string;
    readonly photoType: PhotoType;
    readonly displayName: string;

    get(member: string): MemberType;
    set(member: string, value: string): void;
    commitModify(callback: AsyncCallback<void>): void;
    commitModify(): Promise<void>;
    open(mode: string, callback: AsyncCallback<number | undefined>): void;
    open(mode: string): Promise<number>;
    close(fd: number, callback: AsyncCallback<void>): void;
    close(fd: number): Promise<void>;
    getThumbnail(callback: AsyncCallback<image.PixelMap | undefined>): void;
    getThumbnail(size: image.Size, callback: AsyncCallback<image.PixelMap | undefined>): void;
    getThumbnail(size?: image.Size): Promise<image.PixelMap>;
    setFavorite(favoriteState: boolean, callback: AsyncCallback<void>): void;
    setFavorite(favoriteState: boolean): Promise<void>;
    setHidden(hiddenState: boolean, callback: AsyncCallback<void>): void;
    setHidden(hiddenState: boolean): Promise<void>;
    setUserComment(userComment: string, callback: AsyncCallback<void>): void;
    setUserComment(userComment: string): Promise<void>;
    getAnalysisData(analysisType: AnalysisType): Promise<string>;
    requestEditData(callback: AsyncCallback<string | undefined>): void;
    requestEditData(): Promise<string>;
    getEditData(): Promise<MediaAssetEditData>;
    clone(title: string): Promise<PhotoAsset>;
    convertImageFormat(title: string, imageFormat: SupportedImageFormat): Promise<PhotoAsset>;
    requestSource(callback: AsyncCallback<int | undefined>): void;
    requestSource(): Promise<int>;
    commitEditedAsset(editData: string, uri: string, callback: AsyncCallback<void>): void;
    commitEditedAsset(editData: string, uri: string): Promise<void>;
    revertToOriginal(callback: AsyncCallback<void>): void;
    revertToOriginal(): Promise<void>;
    requestPhoto(callback: AsyncCallback<image.PixelMap>): string;
    requestPhoto(options: RequestPhotoOptions, callback: AsyncCallback<image.PixelMap>): string;
    cancelPhotoRequest(requestId: string): void;
    getKeyFrameThumbnail(beginFrameTimeMs: long, type: ThumbnailType): Promise<image.PixelMap>;
    isEdited(callback: AsyncCallback<boolean | undefined>): void;
    isEdited(): Promise<boolean>;
    getThumbnailData(type: ThumbnailType): Promise<ArrayBuffer>;
    getExif(callback: AsyncCallback<string | undefined>): void;
    getExif(): Promise<string>;
    setPending(pendingState: boolean, callback: AsyncCallback<void>): void;
    setPending(pendingState: boolean): Promise<void>;
  }

  class PhotoAssetHandle implements PhotoAsset {
    readonly uri: string = "";
    readonly photoType: PhotoType;
    readonly displayName: string = "";
    readonly captureId: int = 0;
    private nativePhotoAsset: long = 0;

    constructor(context: long) {
      if (this.nativePhotoAsset == 0) {
        this.nativePhotoAsset = context;
      }
    }

    private static native transferToDynamicPhotoAsset(input: PhotoAssetHandle): Any;
    private static native transferToStaticPhotoAsset(input: ESValue): PhotoAssetHandle;

    static transferDynamic(input: Object): Any {
      return PhotoAssetHandle.transferToDynamicPhotoAsset(input as PhotoAssetHandle);
    }
    static transferStatic(input: Any): Object {
      if (input === undefined || input === null) {
        throw new Error("Input is null or undefined");
      }
      let dyESValue: ESValue = ESValue.wrap(input);
      if( dyESValue === undefined || dyESValue === null) {
        throw new Error("Input is not a valid ESValue");
      }
      let output: PhotoAssetHandle = PhotoAssetHandle.transferToStaticPhotoAsset(dyESValue);
      if (output === null) {
        throw new Error("Failed to transfer to PhotoAssetHandle");
      }
      return output as Object;
    }

    native get(member: string): MemberType;

    native set(member: string, value: string): void;

    commitModify(callback: AsyncCallback<void>): void {
      let cb = (): MediaLibraryAniError | undefined => {
        try {
          this.commitModifySync();
          return undefined;
        } catch(e) {
          return e as MediaLibraryAniError;
        }
      }
      taskpool.execute(cb).then((ret: Any): void => {
        if (isNullish(ret)) {
          callback(businessError(0), undefined);
        } else {
          let err = ret as MediaLibraryAniError;
          callback(businessError(err.code as int, err.message), undefined);
        }
      });
    }

    commitModify(): Promise<void> {
      return new Promise<void>((resolve: (v: undefined) => void, reject: (e: BusinessError<void>) => void): void => {
        let cb = (): MediaLibraryAniError | undefined => {
          try {
            this.commitModifySync();
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

    open(mode: string, callback: AsyncCallback<number | undefined>): void {
      let cb = (): MediaLibraryAniError | number => {
        let result: number | MediaLibraryAniError;
        try {
          result = this.openSync(mode);
        } catch(e) {
          result = e as MediaLibraryAniError;
        }
        return result;
      };
      taskpool.execute(cb).then((ret: Any): void => {
        if (ret instanceof MediaLibraryAniError) {
          let err = ret as MediaLibraryAniError;
          callback(businessError(err.code as int, err.message), undefined);
        } else {
          if (isNullish(ret)) {
            callback(businessError(-1, "Operation failed"), undefined);
          } else {
            callback(businessError(0), ret as number);
          }
        }
      });
    }

    open(mode: string): Promise<number> {
      return new Promise<number>((resolve: (v: number) => void, reject: RejectString) => {
        let cb = (): number | MediaLibraryAniError => {
          let result: number | MediaLibraryAniError;
          try {
            result = this.openSync(mode);
          } catch(e) {
            result = e as MediaLibraryAniError;
          }
          return result;
        };
        taskpool.execute(cb).then((ret: Any): void => {
          if (ret instanceof MediaLibraryAniError) {
            let err = ret as MediaLibraryAniError;
            reject(businessError<string>(err.code as int, err.message));
          } else {
            if (isNullish(ret)) {
              reject(businessError<string>(-1, "Operation failed"));
            } else {
              resolve(ret as number);
            }
          }
        });
      });
    }

    close(fd: number, callback: AsyncCallback<void>): void {
      let cb = (): MediaLibraryAniError | undefined => {
        try {
          this.closeSync(fd);
          return undefined;
        } catch(e) {
          return e as MediaLibraryAniError;
        }
      }
      taskpool.execute(cb).then((ret: Any): void => {
        if (isNullish(ret)) {
          callback(businessError(0), undefined);
        } else {
          let err = ret as MediaLibraryAniError;
          callback(businessError(err.code as int, err.message), undefined);
        }
      });
    }

    close(fd: number): Promise<void> {
      return new Promise<void>((resolve: (v: undefined) => void, reject: (e: BusinessError<void>) => void): void => {
        let cb = (): MediaLibraryAniError | undefined => {
          try {
            this.closeSync(fd);
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

    getThumbnail(callback: AsyncCallback<image.PixelMap | undefined>): void {
      let cb = (): MediaLibraryAniError | image.PixelMap => {
        let result: MediaLibraryAniError | image.PixelMap;
        try {
          result = this.getThumbnailSync();
        } catch(e) {
          result = e as MediaLibraryAniError;
        }
        return result;
      };
      taskpool.execute(cb).then((ret: Any): void => {
        if (ret instanceof MediaLibraryAniError) {
          let err = ret as MediaLibraryAniError;
          callback(businessError(err.code as int, err.message), undefined);
        } else {
          if (isNullish(ret)) {
            callback(businessError(-1, "Operation failed"), undefined);
          } else {
            callback(businessError(0), ret as image.PixelMap);
          }
        }
      });
    }

    getThumbnail(size: image.Size, callback: AsyncCallback<image.PixelMap | undefined>): void {
      let cb = (): MediaLibraryAniError | image.PixelMap => {
        let result: MediaLibraryAniError | image.PixelMap;
        try {
          result = this.getThumbnailSync(size);
        } catch(e) {
          result = e as MediaLibraryAniError;
        }
        return result;
      };
      taskpool.execute(cb).then((ret: Any): void => {
        if (ret instanceof MediaLibraryAniError) {
          let err = ret as MediaLibraryAniError;
          callback(businessError(err.code as int, err.message), undefined);
        } else {
          if (isNullish(ret)) {
            callback(businessError(-1, "Operation failed"), undefined);
          } else {
            callback(businessError(0), ret as image.PixelMap);
          }
        }
      });
    }

    getThumbnail(size?: image.Size): Promise<image.PixelMap> {
      return new Promise<image.PixelMap>((resolve: (v: image.PixelMap) => void, reject: RejectString) => {
        let cb = (): image.PixelMap | MediaLibraryAniError => {
          let result: image.PixelMap | MediaLibraryAniError;
          try {
            result = this.getThumbnailSync(size);
          } catch(e) {
            result = e as MediaLibraryAniError;
          }
          return result;
        };
        taskpool.execute(cb).then((ret: Any): void => {
          if (ret instanceof MediaLibraryAniError) {
            let err = ret as MediaLibraryAniError;
            reject(businessError<string>(err.code as int, err.message));
          } else {
            if (isNullish(ret)) {
              reject(businessError<string>(-1, "Operation failed"));
            } else {
              resolve(ret as image.PixelMap);
            }
          }
        });
      });
    }

    setFavorite(favoriteState: boolean, callback: AsyncCallback<void>): void {
      let cb = (): MediaLibraryAniError | undefined => {
        try {
          this.setFavoriteSync(favoriteState);
          return undefined;
        } catch(e) {
          return e as MediaLibraryAniError;
        }
      }
      taskpool.execute(cb).then((ret: Any): void => {
        if (isNullish(ret)) {
          callback(businessError(0), undefined);
        } else {
          let err = ret as MediaLibraryAniError;
          callback(businessError(err.code as int, err.message), undefined);
        }
      });
    }

    setFavorite(favoriteState: boolean): Promise<void> {
      return new Promise<void>((resolve: (v: undefined) => void, reject: (e: BusinessError<void>) => void): void => {
        let cb = (): MediaLibraryAniError | undefined => {
          try {
            this.setFavoriteSync(favoriteState);
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

    setHidden(hiddenState: boolean, callback: AsyncCallback<void>): void {
      let cb = (): MediaLibraryAniError | undefined => {
        try {
          this.setHiddenSync(hiddenState);
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

    setHidden(hiddenState: boolean): Promise<void> {
      return new Promise<void>((resolve: (v: undefined) => void, reject: (e: BusinessError<void>) => void): void => {
        let cb = (): MediaLibraryAniError | undefined => {
          try {
            this.setHiddenSync(hiddenState);
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

    setUserComment(userComment: string, callback: AsyncCallback<void>): void {
      let cb = (): MediaLibraryAniError | undefined => {
        try {
          this.setUserCommentSync(userComment);
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

    setUserComment(userComment: string): Promise<void> {
      return new Promise<void>((resolve: (v: undefined) => void, reject: (e: BusinessError<void>) => void): void => {
        let cb = (): MediaLibraryAniError | undefined => {
          try {
            this.setUserCommentSync(userComment);
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

    getAnalysisData(analysisType: AnalysisType): Promise<string> {
      return new Promise<string>((resolve: (v: string) => void, reject: RejectString) => {
        let cb = (): string | MediaLibraryAniError => {
          let result: string | MediaLibraryAniError;
          try {
            result = this.getAnalysisDataSync(analysisType);
          } catch(e) {
            result = e as MediaLibraryAniError;
          }
          return result;
        };
        taskpool.execute(cb).then((ret: Any): void => {
          if (ret instanceof MediaLibraryAniError) {
            let err = ret as MediaLibraryAniError;
            reject(businessError<string>(err.code as int, err.message));
          } else {
            if (isNullish(ret)) {
              reject(businessError<string>(-1, "Operation failed"));
            } else {
              resolve(ret as string);
            }
          }
        });
      });
    }

    requestEditData(callback: AsyncCallback<string | undefined>): void {
      let cb = (): MediaLibraryAniError | string => {
        let result: string | MediaLibraryAniError;
        try {
          result = this.requestEditDataSync();
        } catch(e) {
          result = e as MediaLibraryAniError;
        }
        return result;
      };

      taskpool.execute(cb).then((ret: Any): void => {
        if (ret instanceof MediaLibraryAniError) {
          let err = ret as MediaLibraryAniError;
          callback(businessError(err.code as int, err.message), undefined);
        } else {
          if (isNullish(ret)) {
            callback(businessError(-1, "Operation failed"), undefined);
          } else {
            callback(businessError(0), ret as string);
          }
        }
      });
    }

    requestEditData(): Promise<string> {
      return new Promise<string>((resolve: (v: string) => void, reject: RejectString) => {
        let cb = (): string | MediaLibraryAniError => {
          let result: string | MediaLibraryAniError;
          try {
            result = this.requestEditDataSync();
          } catch(e) {
            result = e as MediaLibraryAniError;
          }
          return result;
        };
        taskpool.execute(cb).then((ret: Any): void => {
          if (ret instanceof MediaLibraryAniError) {
            let err = ret as MediaLibraryAniError;
            reject(businessError<string>(err.code as int, err.message));
          } else {
            if (isNullish(ret)) {
              reject(businessError<string>(-1, "Operation failed"));
            } else {
              resolve(ret as string);
            }
          }
        });
      });
    }

    getEditData(): Promise<MediaAssetEditData> {
      return new Promise<MediaAssetEditData>((resolve: (v: MediaAssetEditData) => void, reject: RejectString) => {
        let cb = (): MediaAssetEditData | MediaLibraryAniError => {
          let result: MediaAssetEditData | MediaLibraryAniError;
          try {
            result = this.getEditDataSync();
          } catch(e) {
            result = e as MediaLibraryAniError;
          }
          return result;
        };
        taskpool.execute(cb).then((ret: Any): void => {
          if (ret instanceof MediaLibraryAniError) {
            let err = ret as MediaLibraryAniError;
            reject(businessError<string>(err.code as int, err.message));
          } else {
            if (isNullish(ret)) {
              reject(businessError<string>(-1, "Operation failed"));
            } else {
              resolve(ret as MediaAssetEditData);
            }
          }
        });
      });
    }

    clone(title: string): Promise<PhotoAsset> {
      return new Promise<PhotoAsset>((resolve: (v: PhotoAsset) => void, reject: RejectString) => {
        let cb = (): PhotoAsset | MediaLibraryAniError => {
          let result: PhotoAsset | MediaLibraryAniError;
          try {
            result = this.cloneSync(title);
          } catch(e) {
            result = e as MediaLibraryAniError;
          }
          return result;
        };
        taskpool.execute(cb).then((ret: Any): void => {
          if (ret instanceof MediaLibraryAniError) {
            let err = ret as MediaLibraryAniError;
            reject(businessError<string>(err.code as int, err.message));
          } else {
            if (isNullish(ret)) {
              reject(businessError<string>(-1, "Operation failed"));
            } else {
              resolve(ret as PhotoAsset);
            }
          }
        });
      });
    }

    convertImageFormat(title: string, imageFormat: SupportedImageFormat): Promise<PhotoAsset> {
      return new Promise<PhotoAsset>((resolve: (v: PhotoAsset) => void, reject: RejectString) => {
        let cb = (): PhotoAsset | MediaLibraryAniError => {
          let result: PhotoAsset | MediaLibraryAniError;
          try {
            result = this.convertImageFormatInner(title, imageFormat);
          } catch(e) {
            result = e as MediaLibraryAniError;
          }
          return result;
        };
        taskpool.execute(cb).then((ret: Any): void => {
          if (ret instanceof MediaLibraryAniError) {
            let err = ret as MediaLibraryAniError;
            reject(businessError<string>(err.code as int, err.message));
          } else {
            if (isNullish(ret)) {
              reject(businessError<string>(-1, "Operation failed"));
            } else {
              resolve(ret as PhotoAsset);
            }
          }
        });
      });
    }

    requestSource(callback: AsyncCallback<int | undefined>): void {
      let cb = (): MediaLibraryAniError | int => {
        let result: int | MediaLibraryAniError;
        try {
          result = this.requestSourceSync() ;
        } catch(e) {
          result = e as MediaLibraryAniError;
        }
        return result;
      };
      taskpool.execute(cb).then((ret: Any): void => {
        if (ret instanceof MediaLibraryAniError) {
          let err = ret as MediaLibraryAniError;
          callback(businessError(err.code as int, err.message), undefined);
        } else {
           if (isNullish(ret)) {
            callback(businessError(-1, "Operation failed"), undefined);
          } else {
            callback(businessError(0), ret as int);
          }
        }
      });
    }

    requestSource(): Promise<int> {
      return new Promise<int>((resolve: (v: int) => void, reject: RejectString) => {
        let cb = (): int | MediaLibraryAniError => {
          let result: int | MediaLibraryAniError;
          try {
            result =  this.requestSourceSync();
          } catch(e) {
            result = e as MediaLibraryAniError;
          }
          return result;
        };
        taskpool.execute(cb).then((ret: Any): void => {
          if (ret instanceof MediaLibraryAniError) {
            let err = ret as MediaLibraryAniError;
            reject(businessError<string>(err.code as int, err.message));
          } else {
            if (isNullish(ret)) {
              reject(businessError<string>(-1, "Operation failed"));
            } else {
              resolve(ret as int);
            }
          }
        });
      });
    }

    commitEditedAsset(editData: string, uri: string, callback: AsyncCallback<void>): void {
      let cb = (): MediaLibraryAniError | undefined => {
        try {
          this.commitEditedAssetSync(editData, uri);
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

    commitEditedAsset(editData: string, uri: string): Promise<void> {
      return new Promise<void>((resolve: (v: undefined) => void, reject: (e: BusinessError<void>) => void): void => {
        let cb = (): MediaLibraryAniError | undefined => {
          try {
            this.commitEditedAssetSync(editData, uri);
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

    revertToOriginal(callback: AsyncCallback<void>): void {
      let cb = (): MediaLibraryAniError | undefined => {
        try {
          this.revertToOriginalSync();
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

    revertToOriginal(): Promise<void> {
      return new Promise<void>((resolve: (v: undefined) => void, reject: (e: BusinessError<void>) => void): void => {
        let cb = (): MediaLibraryAniError | undefined => {
          try {
            this.revertToOriginalSync();
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

    requestPhoto(callback: AsyncCallback<image.PixelMap>): string {
      return this.requestPhotoSync(callback);
    }

    requestPhoto(options: RequestPhotoOptions, callback: AsyncCallback<image.PixelMap>): string {
      return this.requestPhotoSync(callback, options);
    }

    getKeyFrameThumbnail(beginFrameTimeMs: long, type: ThumbnailType): Promise<image.PixelMap> {
      return new Promise<image.PixelMap>((resolve: (v: image.PixelMap) => void, reject: RejectString) => {
        let cb = (): image.PixelMap | MediaLibraryAniError => {
          let result: image.PixelMap | MediaLibraryAniError;
          try {
            result = this.getKeyFrameThumbnailSync(beginFrameTimeMs, type);
          } catch(e) {
            result = e as MediaLibraryAniError;
          }
          return result;
        };
        taskpool.execute(cb).then((ret: Any): void => {
          if (ret instanceof MediaLibraryAniError) {
            let err = ret as MediaLibraryAniError;
            reject(businessError<string>(err.code as int, err.message));
          } else {
            if (isNullish(ret)) {
              reject(businessError<string>(-1, "Operation failed"));
            } else {
              resolve(ret as image.PixelMap);
            }
          }
        });
      });
    }

    isEdited(callback: AsyncCallback<boolean | undefined>): void {
      let cb = (): MediaLibraryAniError | boolean => {
        let result: boolean | MediaLibraryAniError;
        try {
          result = this.isEditedSync();
        } catch(e) {
          result = e as MediaLibraryAniError;
        }
        return result;
      };
      taskpool.execute(cb).then((ret: Any): void => {
        if (ret instanceof MediaLibraryAniError) {
          let err = ret as MediaLibraryAniError;
          callback(businessError(err.code as int, err.message), undefined);
        } else {
          if (isNullish(ret)) {
            callback(businessError(-1, "Operation failed"), undefined);
          } else {
            callback(businessError(0), ret as boolean);
          }
        }
      });
    }

    isEdited(): Promise<boolean> {
      return new Promise<boolean>((resolve: (v: boolean) => void, reject: RejectString) => {
        let cb = (): boolean | MediaLibraryAniError => {
          let result: boolean | MediaLibraryAniError;
          try {
            result = this.isEditedSync();
          } catch(e) {
            result = e as MediaLibraryAniError;
          }
          return result;
        };
        taskpool.execute(cb).then((ret: Any): void => {
          if (ret instanceof MediaLibraryAniError) {
            let err = ret as MediaLibraryAniError;
            reject(businessError<string>(err.code as int, err.message));
          } else {
            if (isNullish(ret)) {
              reject(businessError<string>(-1, "Operation failed"));
            } else {
              resolve(ret as boolean);
            }
          }
        });
      });
    }

    getThumbnailData(type: ThumbnailType): Promise<ArrayBuffer> {
      return new Promise<ArrayBuffer>((resolve: (v: ArrayBuffer) => void, reject: RejectString) => {
        let cb = (): ArrayBuffer | MediaLibraryAniError => {
          let result: ArrayBuffer | MediaLibraryAniError;
          try {
            result = this.getThumbnailDataSync(type);
          } catch(e) {
            result = e as MediaLibraryAniError;
          }
          return result;
        };
        taskpool.execute(cb).then((ret: Any): void => {
          if (ret instanceof MediaLibraryAniError) {
            let err = ret as MediaLibraryAniError;
            reject(businessError<string>(err.code as int, err.message));
          } else {
            if (isNullish(ret)) {
              reject(businessError<string>(-1, "Operation failed"));
            } else {
              resolve(ret as ArrayBuffer);
            }
          }
        });
      });
    }

    getExif(callback: AsyncCallback<string | undefined>): void {
      let cb = (): MediaLibraryAniError | string => {
        let result: string | MediaLibraryAniError;
        try {
          result = this.getExifSync();
        } catch(e) {
          result = e as MediaLibraryAniError;
        }
        return result;
      };
      taskpool.execute(cb).then((ret: Any): void => {
        if (ret instanceof MediaLibraryAniError) {
          let err = ret as MediaLibraryAniError;
          callback(businessError(err.code as int, err.message), undefined);
        } else {
          if (isNullish(ret)) {
            callback(businessError(-1, "Operation failed"), undefined);
          } else {
            callback(businessError(0), ret as string);
          }
        }
      });
    }

    getExif(): Promise<string> {
      return new Promise<string>((resolve: (v: string) => void, reject: RejectString) => {
        let cb = (): string | MediaLibraryAniError => {
          let result: string | MediaLibraryAniError;
          try {
            result = this.getExifSync();
          } catch(e) {
            result = e as MediaLibraryAniError;
          }
          return result;
        };
        taskpool.execute(cb).then((ret: Any): void => {
          if (ret instanceof MediaLibraryAniError) {
            let err = ret as MediaLibraryAniError;
            reject(businessError<string>(err.code as int, err.message));
          } else {
            if (isNullish(ret)) {
              reject(businessError<string>(-1, "Operation failed"));
            } else {
              resolve(ret as string);
            }
          }
        });
      });
    }

    setPending(pendingState: boolean, callback: AsyncCallback<void>): void {
      let cb = (): MediaLibraryAniError | undefined => {
        try {
          this.setPendingSync(pendingState);
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

    setPending(pendingState: boolean): Promise<void> {
      return new Promise<void>((resolve: (v: undefined) => void, reject: (e: BusinessError<void>) => void): void => {
        let cb = (): MediaLibraryAniError | undefined => {
          try {
            this.setPendingSync(pendingState);
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

    native commitModifySync(): void;
    native openSync(mode: string): number;
    native closeSync(fd: number): void;
    native getThumbnailSync(size?: image.Size): image.PixelMap;
    native setFavoriteSync(favoriteState: boolean): void;
    native setHiddenSync(hiddenState: boolean): void;
    native setUserCommentSync(userComment: string): void;
    native getAnalysisDataSync(analysisType: AnalysisType): string;
    native requestEditDataSync(): string;
    native getEditDataSync(): MediaAssetEditData;
    native cloneSync(title: string): PhotoAsset;
    native convertImageFormatInner(title: string, imageFormat: SupportedImageFormat): PhotoAsset;
    native requestSourceSync(): int;
    native commitEditedAssetSync(editData: string, uri: string): void;
    native revertToOriginalSync(): void;
    native requestPhotoSync(callback: AsyncCallback<image.PixelMap>, options?: RequestPhotoOptions): string;
    native cancelPhotoRequest(requestId: string): void;
    native getKeyFrameThumbnailSync(beginFrameTimeMs: long, type: ThumbnailType): image.PixelMap;
    native isEditedSync(): boolean;
    native getThumbnailDataSync(type: ThumbnailType): ArrayBuffer;
    native getExifSync(): string;
    native setPendingSync(pendingState: boolean): void;
  }

  export enum PhotoKeys {
    URI = 'uri',
    PHOTO_TYPE = 'media_type',
    DISPLAY_NAME = 'display_name',
    SIZE = 'size',
    DATE_ADDED = 'date_added',
    DATE_MODIFIED = 'date_modified',
    DURATION = 'duration',
    WIDTH = 'width',
    HEIGHT = 'height',
    DATE_TAKEN = 'date_taken',
    ORIENTATION = 'orientation',
    FAVORITE = 'is_favorite',
    TITLE = 'title',
    POSITION = 'position',
    DATE_TRASHED = 'date_trashed',
    HIDDEN = 'hidden',
    USER_COMMENT = 'user_comment',
    CAMERA_SHOT_KEY = 'camera_shot_key',
    DATE_YEAR = 'date_year',
    DATE_MONTH = 'date_month',
    DATE_DAY = 'date_day',
    PENDING = 'pending',
    DATE_ADDED_MS = 'date_added_ms',
    DATE_MODIFIED_MS = 'date_modified_ms',
    DATE_TRASHED_MS = 'date_trashed_ms',
    PHOTO_SUBTYPE = 'subtype',
    MOVING_PHOTO_EFFECT_MODE = 'moving_photo_effect_mode',
    DYNAMIC_RANGE_TYPE = 'dynamic_range_type',
    COVER_POSITION = 'cover_position',
    BURST_KEY = 'burst_key',
    THUMBNAIL_READY = 'thumbnail_ready',
    LCD_SIZE = 'lcd_size',
    THM_SIZE = 'thumb_size',
    DETAIL_TIME = 'detail_time',
    DATE_TAKEN_MS = 'date_taken_ms',
    CE_AVAILABLE = 'ce_available',
    SUPPORTED_WATERMARK_TYPE = 'supported_watermark_type',
    THUMBNAIL_VISIBLE = 'thumbnail_visible',
    IS_CE_AUTO = 'is_auto',
    OWNER_ALBUM_ID = 'owner_album_id',
    IS_RECENT_SHOW = 'is_recent_show',
    MEDIA_SUFFIX = 'media_suffix',
    SUM_SIZE = 'sum(size)',
    CHANGE_TIME = 'change_time',
    VIDEO_MODE = 'video_mode',
    ASPECT_RATIO = 'aspect_ratio',
    EDIT_DATA_EXIST = 'edit_data_exist',
  }

  export enum AlbumKeys {
    URI = 'uri',
    ALBUM_NAME = 'album_name',
    ALBUM_LPATH = 'lpath',
    BUNDLE_NAME = 'bundle_name',
    DATE_MODIFIED = 'date_modified',
    CHANGE_TIME = 'change_time',
    COVER_URI_SOURCE = 'cover_uri_source',
  }

  export enum HiddenPhotosDisplayMode {
    ASSETS_MODE,
    ALBUMS_MODE
  }

  export interface FetchOptions {
    fetchColumns: Array<string>;
    predicates: dataSharePredicates.DataSharePredicates;
  }

  export interface PhotoCreateOptions {
    subtype?: PhotoSubtype;
    cameraShotKey?: string;
    userId?: int;
  }

  export interface PhotoCreationConfig {
    title?: string;
    fileNameExtension: string;
    photoType: PhotoType;
    subtype?: PhotoSubtype;
  }

  export interface PhotoCreationSource {
    bundleName?: string;
    appName?: string;
    appId?: string;
    tokenId?: long;
  }

  export interface CreateOptions {
    title?: string;
    subtype?: PhotoSubtype;
  }

  export interface RequestPhotoOptions {
    size?: image.Size;
    requestPhotoType?: RequestPhotoType;
  }

  export interface FetchResult<T> {
    getCount(): int;
    isAfterLast(): boolean;
    getFirstObject(callback: AsyncCallback<T | undefined>): void;
    getFirstObject(): Promise<T>;
    getNextObject(callback: AsyncCallback<T | undefined>): void;
    getNextObject(): Promise<T>;
    getLastObject(callback: AsyncCallback<T | undefined>): void;
    getLastObject(): Promise<T>;
    getObjectByPosition(index: int, callback: AsyncCallback<T | undefined>): void;
    getObjectByPosition(index: int): Promise<T>;
    getAllObjects(callback: AsyncCallback<Array<T> | undefined>): void;
    getAllObjects(): Promise<Array<T>>;
    close(): void;
  }

  class FetchResultHandle<T> implements FetchResult<T> {
    private nativeValue: long = 0;

    constructor(value: long) {
        this.nativeValue = value;
    }

    private static native transferToDynamicFetchResult(input: Object): Any;
    private static native transferToStaticFetchResult(input: ESValue): Object;

    static transferDynamic(input: Object): Any {
        return FetchResultHandle.transferToDynamicFetchResult(input);
    }
    static transferStatic(input: Any): Object {
      if (input === undefined || input === null) {
        throw new Error("Input is null or undefined");
      }
      let dyESValue: ESValue = ESValue.wrap(input);
      if( dyESValue === undefined || dyESValue === null) {
        throw new Error("Input is not a valid ESValue");
      }
      let output: Object = FetchResultHandle.transferToStaticFetchResult(dyESValue);
      if (output === null) {
        throw new Error("Failed to transfer to FetchResultHandle");
      }
      return output;
    }

    native getCount(): int;
    native isAfterLast(): boolean;
    getFirstObject(callback: AsyncCallback<T | undefined, void>): void {
      let cb = (): T | MediaLibraryAniError => {
        let result: T | MediaLibraryAniError;
        try {
          result = this.getFirstObjectSync();
        } catch(e) {
          result = e as MediaLibraryAniError;
        }
        return result;
      };
      taskpool.execute(cb).then((ret: Any): void => {
        if (ret instanceof MediaLibraryAniError) {
          let err = ret as MediaLibraryAniError;
          callback(businessError(err.code as int, err.message), undefined);
        } else {
          if (isNullish(ret)) {
            callback(businessError(-1), undefined);
          } else {
            callback(businessError(0), ret as T);
          }
        }
      });
    }

    getFirstObject(): Promise<T> {
      return new Promise<T>((resolve: (v: T) => void, reject: RejectString) => {
        let cb = (): T | MediaLibraryAniError => {
          let result: T | MediaLibraryAniError;
          try {
            result = this.getFirstObjectSync();
          } catch(e) {
            result = e as MediaLibraryAniError;
          }
          return result;
        };
        taskpool.execute(cb).then((ret: Any): void => {
          if (ret instanceof MediaLibraryAniError) {
            let err = ret as MediaLibraryAniError;
            reject(businessError<string>(err.code as int, err.message));
          } else {
            if (isNullish(ret)) {
              reject(businessError<string>(-1, "Operation failed"));
            } else {
              resolve(ret as T);
            }
          }
        });
      });
    }

    getNextObject(callback: AsyncCallback<T | undefined, void>): void {
      let cb = (): T | MediaLibraryAniError => {
        let result: T | MediaLibraryAniError;
        try {
          result = this.getNextObjectSync();
        } catch(e) {
          result = e as MediaLibraryAniError;
        }
        return result;
      };
      taskpool.execute(cb).then((ret: Any): void => {
        if (ret instanceof MediaLibraryAniError) {
          let err = ret as MediaLibraryAniError;
          callback(businessError(err.code as int, err.message), undefined);
        } else {
          if (isNullish(ret)) {
            callback(businessError(-1), undefined);
          } else {
            callback(businessError(0), ret as T);
          }
        }
      });
    }

    getNextObject(): Promise<T> {
      return new Promise<T>((resolve: (v: T) => void, reject: RejectString) => {
        let cb = (): T | MediaLibraryAniError => {
          let result: T | MediaLibraryAniError;
          try {
            result = this.getNextObjectSync();
          } catch(e) {
            result = e as MediaLibraryAniError;
          }
          return result;
        };
        taskpool.execute(cb).then((ret: Any): void => {
          if (ret instanceof MediaLibraryAniError) {
            let err = ret as MediaLibraryAniError;
            reject(businessError<string>(err.code as int, err.message));
          } else {
            if (isNullish(ret)) {
              reject(businessError<string>(-1, "Operation failed"));
            } else {
              resolve(ret as T);
            }
          }
        });
      });
    }

    getLastObject(callback: AsyncCallback<T | undefined, void>): void {
      let cb = (): T | MediaLibraryAniError => {
        let result: T | MediaLibraryAniError;
        try {
          result = this.getLastObjectSync();
        } catch(e) {
          result = e as MediaLibraryAniError;
        }
        return result;
      };
      taskpool.execute(cb).then((ret: Any): void => {
        if (ret instanceof MediaLibraryAniError) {
          let err = ret as MediaLibraryAniError;
          callback(businessError(err.code as int, err.message), undefined);
        } else {
          if (isNullish(ret)) {
            callback(businessError(-1), undefined);
          } else {
            callback(businessError(0), ret as T);
          }
        }
      });
    }

    getLastObject(): Promise<T> {
      return new Promise<T>((resolve: (v: T) => void, reject: RejectString) => {
        let cb = (): T | MediaLibraryAniError => {
          let result: T | MediaLibraryAniError;
          try {
            result = this.getLastObjectSync();
          } catch(e) {
            result = e as MediaLibraryAniError;
          }
          return result;
        };
        taskpool.execute(cb).then((ret: Any): void => {
          if (ret instanceof MediaLibraryAniError) {
            let err = ret as MediaLibraryAniError;
            reject(businessError<string>(err.code as int, err.message));
          } else {
            if (isNullish(ret)) {
              reject(businessError<string>(-1, "Operation failed"));
            } else {
              resolve(ret as T);
            }
          }
        });
      });
    }

    getObjectByPosition(index: int, callback: AsyncCallback<T | undefined>): void {
      let cb = (): MediaLibraryAniError | T => {
        let result: MediaLibraryAniError | T;
        try {
          result = this.getObjectByPositionSync(index);
        } catch(e) {
          result = e as MediaLibraryAniError;
        }
        return result;
      };
      taskpool.execute(cb).then((ret: Any): void => {
        if (ret instanceof MediaLibraryAniError) {
          let err = ret as MediaLibraryAniError;
          callback(businessError(err.code as int, err.message), undefined);
        } else {
          if (isNullish(ret)) {
            callback(businessError(-1, "Operation failed"), undefined);
          } else {
            callback(businessError(0), ret as T);
          }
        }
      });
    }

    getObjectByPosition(index: int): Promise<T> {
      return new Promise<T>((resolve: (v: T) => void, reject: RejectString) => {
        let cb = (): T | MediaLibraryAniError => {
          let result: T | MediaLibraryAniError;
          try {
            result = this.getObjectByPositionSync(index);
          } catch(e) {
            result = e as MediaLibraryAniError;
          }
          return result;
        };
        taskpool.execute(cb).then((ret: Any): void => {
          if (ret instanceof MediaLibraryAniError) {
            let err = ret as MediaLibraryAniError;
            reject(businessError<string>(err.code as int, err.message));
          } else {
            if (isNullish(ret)) {
              reject(businessError<string>(-1, "Operation failed"));
            } else {
              resolve(ret as T);
            }
          }
        });
      });
    }

    getAllObjects(callback: AsyncCallback<Array<T> | undefined, void>): void {
      let cb = (): Array<T> | MediaLibraryAniError => {
        let result: Array<T> | MediaLibraryAniError;
        try {
          result = this.getAllObjectsSync();
        } catch(e) {
          result = e as MediaLibraryAniError;
        }
        return result;
      };
      taskpool.execute(cb).then((ret: Any): void => {
        if (ret instanceof MediaLibraryAniError) {
          let err = ret as MediaLibraryAniError;
          callback(businessError(err.code as int, err.message), undefined);
        } else {
          if (isNullish(ret)) {
            callback(businessError(-1), undefined);
          } else {
            callback(businessError(0), ret as Array<T>);
          }
        }
      });
    }

    getAllObjects(): Promise<Array<T>> {
      return new Promise<Array<T>>((resolve: (v: Array<T>) => void, reject: RejectString) => {
        let blockQueue: containers.BlockingQueue<Array<T> | MediaLibraryAniError> =
          new containers.ArrayBlockingQueue<Array<T> | MediaLibraryAniError>(2);
        let cb = (): Array<T> | MediaLibraryAniError => {
        let result: Array<T> | MediaLibraryAniError;
          try {
            result = this.getAllObjectsSync();
          } catch(e) {
            result = e as MediaLibraryAniError;
          }
          blockQueue.push(result);
          return result;
        };
        taskpool.execute(cb).then((ret: Any): void => {
          if (ret instanceof MediaLibraryAniError) {
            let err = ret as MediaLibraryAniError;
            reject(businessError<string>(err.code as int, err.message));
          } else {
            if (isNullish(ret)) {
              reject(businessError<string>(-1, "Operation failed"));
            } else {
              resolve(ret as Array<T>);
            }
          }
        });
        blockQueue.pop();
      });
    }

    native close(): void;

    native getFirstObjectSync(): T;
    native getNextObjectSync(): T;
    native getLastObjectSync(): T;
    native getAllObjectsSync(): Array<T>;
    native getObjectByPositionSync(index: int): T;
  }

  export enum AlbumType {
    USER = 0,
    SYSTEM = 1024,
    SOURCE = 2048,
    SMART = 4096
  }

  export enum AlbumSubtype {
    USER_GENERIC = 1,
    FAVORITE = 1025,
    VIDEO,
    HIDDEN,
    TRASH,
    SCREENSHOT,
    CAMERA,
    IMAGE = 1031,
    CLOUD_ENHANCEMENT = 1032,
    SOURCE_GENERIC = 2049,
    CLASSIFY = 4097,
    GEOGRAPHY_LOCATION = 4099,
    GEOGRAPHY_CITY,
    SHOOTING_MODE,
    PORTRAIT,
    GROUP_PHOTO,
    HIGHLIGHT = 4104,
    HIGHLIGHT_SUGGESTIONS,
    PET = 4106,
    ANY = 2147483647
  }

  export enum RequestPhotoType {
    REQUEST_ALL_THUMBNAILS = 0,
    REQUEST_FAST_THUMBNAIL,
    REQUEST_QUALITY_THUMBNAIL
  }

  enum CoverUriSource {
    DEFAULT_COVER = 0,
    MANUAL_COVER = 1
  }

  export interface AbsAlbum {
    readonly albumType: AlbumType;
    readonly albumSubtype: AlbumSubtype;
    albumName: string;
    readonly albumUri: string;
    readonly count: int;
    readonly coverUri: string;
    readonly lpath?: string;
    readonly changeTime?: long;
    readonly coverUriSource?: CoverUriSource;

    getAssets(options: FetchOptions, callback: AsyncCallback<FetchResult<PhotoAsset> | undefined>): void;
    getAssets(options: FetchOptions): Promise<FetchResult<PhotoAsset>>;
    getSharedPhotoAssets(options: FetchOptions): Array<SharedPhotoAsset>;
  }

  enum NotifyChangeType {
    NOTIFY_CHANGE_ADD = 0,
    NOTIFY_CHANGE_UPDATE = 1,
    NOTIFY_CHANGE_REMOVE = 2,
    NOTIFY_CHANGE_YUV_READY = 3,
  }

  export interface Album extends AbsAlbum {
    readonly imageCount?: int;
    readonly videoCount?: int;
    readonly dateAdded?: long;
    readonly dateModified?: long;

    commitModify(callback: AsyncCallback<void>): void;
    commitModify(): Promise<void>;
    addAssets(assets: Array<PhotoAsset>, callback: AsyncCallback<void>): void;
    addAssets(assets: Array<PhotoAsset>): Promise<void>;
    removeAssets(assets: Array<PhotoAsset>, callback: AsyncCallback<void>): void;
    removeAssets(assets: Array<PhotoAsset>): Promise<void>;
    recoverAssets(assets: Array<PhotoAsset>, callback: AsyncCallback<void>): void;
    recoverAssets(assets: Array<PhotoAsset>): Promise<void>;
    deleteAssets(assets: Array<PhotoAsset>, callback: AsyncCallback<void>): void;
    deleteAssets(assets: Array<PhotoAsset>): Promise<void>;
    setCoverUri(uri: string, callback: AsyncCallback<void>): void;
    setCoverUri(uri: string): Promise<void>;
    getFaceId(): Promise<string>;
  }

  class AlbumHandle implements Album {
    readonly albumType: AlbumType;
    readonly albumSubtype: AlbumSubtype;
    albumName: string = '';
    readonly albumUri: string = '';
    readonly count: int;
    readonly coverUri: string = '';
    readonly lpath: string | undefined = undefined;
    readonly imageCount: int | undefined = undefined;
    readonly videoCount: int | undefined = undefined;
    readonly dateAdded: long | undefined = undefined;
    readonly dateModified: long | undefined = undefined;
    readonly changeTime: long | undefined = undefined;
    readonly coverUriSource: CoverUriSource | undefined = undefined;
    private nativePhotoAlbum: long = 0;

    constructor(nativePtr: long) {
      if (this.nativePhotoAlbum == 0) {
        this.nativePhotoAlbum = nativePtr;
        this.imageCount = this.getImageCount();
        this.videoCount = this.getVideoCount();
        this.dateAdded = this.getdateAdded();
        this.dateModified = this.getdateModified();
        this.changeTime = this.getChangeTime();
      }
    }


    private static native transferToDynamicAlbum(input: AlbumHandle): Any;
    private static native transferToStaticAlbum(input: ESValue): AlbumHandle;

    static transferDynamic(input: Object): Any {
      return AlbumHandle.transferToDynamicAlbum(input as AlbumHandle);
    }
    static transferStatic(input: Any): Object {
      if (input === undefined || input === null) {
        throw new Error("Input is null or undefined");
      }
      let dyESValue: ESValue = ESValue.wrap(input);
      if( dyESValue === undefined || dyESValue === null) {
        throw new Error("Input is not a valid ESValue");
      }
      let output: AlbumHandle = AlbumHandle.transferToStaticAlbum(dyESValue);
      if (output === null) {
        throw new Error("Failed to transfer to AlbumHandle");
      }
      return output as Object;
    }

    getAssets(options: FetchOptions, callback: AsyncCallback<FetchResult<PhotoAsset> | undefined>): void {
      let cb = (): MediaLibraryAniError | FetchResult<PhotoAsset> => {
        let result: MediaLibraryAniError | FetchResult<PhotoAsset>;
        try {
          result = this.getAssetsInner(options);
        } catch(e) {
          result = e as MediaLibraryAniError;
        }
        return result;
      };
      taskpool.execute(cb).then((ret: Any): void => {
        if (ret instanceof MediaLibraryAniError) {
          let err = ret as MediaLibraryAniError;
          callback(businessError(err.code as int, err.message), undefined);
        } else {
          if (isNullish(ret)) {
            callback(businessError(-1, "Operation failed"), undefined);
          } else {
            callback(businessError(0), ret as FetchResult<PhotoAsset>);
          }
        }
      });
    }

    getAssets(options: FetchOptions): Promise<FetchResult<PhotoAsset>> {
      return new Promise<FetchResult<PhotoAsset>>((resolve: (v: FetchResult<PhotoAsset>) => void,
        reject: RejectString) => {
        let cb = (): FetchResult<PhotoAsset> | MediaLibraryAniError => {
          let result: FetchResult<PhotoAsset> | MediaLibraryAniError;
          try {
            result = this.getAssetsInner(options);
          } catch(e) {
            result = e as MediaLibraryAniError;
          }
          return result;
        };
        taskpool.execute(cb).then((ret: Any): void => {
          if (ret instanceof MediaLibraryAniError) {
            let err = ret as MediaLibraryAniError;
            reject(businessError<string>(err.code as int, err.message));
          } else {
            if (isNullish(ret)) {
              reject(businessError<string>(-1, "Operation failed"));
            } else {
              resolve(ret as FetchResult<PhotoAsset>);
            }
          }
        });
      });
    }

    commitModify(callback: AsyncCallback<void>): void {
      let cb = (): MediaLibraryAniError | undefined => {
        try {
          this.commitModifyInner();
          return undefined;
        } catch(e) {
          return e as MediaLibraryAniError;
        }
      }
      taskpool.execute(cb).then((ret: Any): void => {
        if (isNullish(ret)) {
          callback(businessError(0), undefined);
        } else {
          let err = ret as MediaLibraryAniError;
          callback(businessError(err.code as int, err.message), undefined);
        }
      });
    }

    commitModify(): Promise<void> {
      return new Promise<void>((resolve: (v: undefined) => void, reject: (e: BusinessError<void>) => void): void => {
        let cb = (): MediaLibraryAniError | undefined => {
          try {
            this.commitModifyInner();
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

    addAssets(assets: Array<PhotoAsset>, callback: AsyncCallback<void>): void {
      let cb = (): MediaLibraryAniError | undefined => {
        try {
          this.addAssetsInner(assets);
          return undefined;
        } catch(e) {
          return e as MediaLibraryAniError;
        }
      }
      taskpool.execute(cb).then((ret: Any): void => {
        if (isNullish(ret)) {
          callback(businessError(0), undefined);
        } else {
          let err = ret as MediaLibraryAniError;
          callback(businessError(err.code as int, err.message), undefined);
        }
      });
    }

    addAssets(assets: Array<PhotoAsset>): Promise<void> {
      return new Promise<void>((resolve: (v: undefined) => void, reject: (e: BusinessError<void>) => void): void => {
        let cb = (): MediaLibraryAniError | undefined => {
          try {
            this.addAssetsInner(assets);
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

    removeAssets(assets: Array<PhotoAsset>, callback: AsyncCallback<void>): void {
      let cb = (): MediaLibraryAniError | undefined => {
        try {
          this.removeAssetsInner(assets);
          return undefined;
        } catch(e) {
          return e as MediaLibraryAniError;
        }
      }
      taskpool.execute(cb).then((ret: Any): void => {
        if (isNullish(ret)) {
          callback(businessError(0), undefined);
        } else {
          let err = ret as MediaLibraryAniError;
          callback(businessError(err.code as int, err.message), undefined);
        }
      });
    }

    removeAssets(assets: Array<PhotoAsset>): Promise<void> {
      return new Promise<void>((resolve: (v: undefined) => void, reject: (e: BusinessError<void>) => void): void => {
        let cb = (): MediaLibraryAniError | undefined => {
          try {
            this.removeAssetsInner(assets);
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

    recoverAssets(assets: Array<PhotoAsset>, callback: AsyncCallback<void>): void {
      let cb = (): MediaLibraryAniError | undefined => {
        try {
          this.recoverAssetsInner(assets);
          return undefined;
        } catch(e) {
          return e as MediaLibraryAniError;
        }
      }
      taskpool.execute(cb).then((ret: Any): void => {
        if (isNullish(ret)) {
          callback(businessError(0), undefined);
        } else {
          let err = ret as MediaLibraryAniError;
          callback(businessError(err.code as int, err.message), undefined);
        }
      });
    }

    recoverAssets(assets: Array<PhotoAsset>): Promise<void> {
      return new Promise<void>((resolve: (v: undefined) => void, reject: (e: BusinessError<void>) => void): void => {
        let cb = (): MediaLibraryAniError | undefined => {
          try {
            this.recoverAssetsInner(assets);
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

    deleteAssets(assets: Array<PhotoAsset>, callback: AsyncCallback<void>): void {
      let cb = (): MediaLibraryAniError | undefined => {
        try {
          this.deleteAssetsInner(assets);
          return undefined;
        } catch(e) {
          return e as MediaLibraryAniError;
        }
      }
      taskpool.execute(cb).then((ret: Any): void => {
        if (isNullish(ret)) {
          callback(businessError(0), undefined);
        } else {
          let err = ret as MediaLibraryAniError;
          callback(businessError(err.code as int, err.message), undefined);
        }
      });
    }

    deleteAssets(assets: Array<PhotoAsset>): Promise<void> {
      return new Promise<void>((resolve: (v: undefined) => void, reject: (e: BusinessError<void>) => void): void => {
        let cb = (): MediaLibraryAniError | undefined => {
          try {
            this.deleteAssetsInner(assets);
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

    setCoverUri(uri: string, callback: AsyncCallback<void>): void {
      let cb = (): MediaLibraryAniError | undefined => {
        try {
          this.setCoverUriInner(uri);
          return undefined;
        } catch(e) {
          return e as MediaLibraryAniError;
        }
      }
      taskpool.execute(cb).then((ret: Any): void => {
        if (isNullish(ret)) {
          callback(businessError(0), undefined);
        } else {
          let err = ret as MediaLibraryAniError;
          callback(businessError(err.code as int, err.message), undefined);
        }
      });
    }

    setCoverUri(uri: string): Promise<void> {
      return new Promise<void>((resolve: (v: undefined) => void, reject: (e: BusinessError<void>) => void): void => {
        let cb = (): MediaLibraryAniError | undefined => {
          try {
            this.setCoverUriInner(uri);
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

    getFaceId(): Promise<string> {
      return new Promise<string>((resolve: (v: string) => void, reject: RejectString) => {
        let cb = (): string | MediaLibraryAniError => {
          let result: string | MediaLibraryAniError;
          try {
            result = this.getFaceIdInner();
          } catch(e) {
            result = e as MediaLibraryAniError;
          }
          return result;
        };
        taskpool.execute(cb).then((ret: Any): void => {
          if (ret instanceof MediaLibraryAniError) {
            let err = ret as MediaLibraryAniError;
            reject(businessError<string>(err.code as int, err.message));
          } else {
            if (isNullish(ret)) {
              reject(businessError<string>(-1, "Operation failed"));
            } else {
              resolve(ret as string);
            }
          }
        });
      });
    }

    native getAssetsInner(options: FetchOptions): FetchResult<PhotoAsset>;
    native getAssetsSync(options: FetchOptions): Array<PhotoAsset>;
    native commitModifyInner(): void;
    native addAssetsInner(assets: Array<PhotoAsset>): void;
    native removeAssetsInner(assets: Array<PhotoAsset>): void;
    native recoverAssetsInner(assets: Array<PhotoAsset>): void;
    native deleteAssetsInner(assets: Array<PhotoAsset>): void;
    native setCoverUriInner(uri: string): void;
    native getFaceIdInner(): string;
    native getImageCount(): int;
    native getVideoCount(): int;
    native getSharedPhotoAssets(options: FetchOptions): Array<SharedPhotoAsset>;
    native getdateAdded(): long;
    native getdateModified(): long;
    native getChangeTime(): long;
  }

  export interface PhotoAccessHelper {
    getAssets(options: FetchOptions, callback: AsyncCallback<FetchResult<PhotoAsset> | undefined>): void;
    getAssets(options: FetchOptions): Promise<FetchResult<PhotoAsset>>;
    getAssetsSync(options: FetchOptions): Array<PhotoAsset>;
    getFileAssetsInfo(options: FetchOptions): Array<FileAssetInfo>;
    startThumbnailCreationTask(predicate: dataSharePredicates.DataSharePredicates,
      callback: AsyncCallback<void>): int;
    stopThumbnailCreationTask(taskId: int): void;
    startCreateThumbnailTask(predicate: dataSharePredicates.DataSharePredicates): int;
    getBurstAssets(burstKey: string, options: FetchOptions): Promise<FetchResult<PhotoAsset>>;
    createAsset(displayName: string, callback: AsyncCallback<PhotoAsset | undefined>): void;
    createAsset(displayName: string): Promise<PhotoAsset>;
    createAsset(displayName: string, options: PhotoCreateOptions): Promise<PhotoAsset>;
    createAsset(displayName: string, options: PhotoCreateOptions,
      callback: AsyncCallback<PhotoAsset | undefined>): void;
    createAsset(photoType: PhotoType, extension: string, options: CreateOptions,
      callback: AsyncCallback<string | undefined>): void;
    createAsset(photoType: PhotoType, extension: string, callback: AsyncCallback<string | undefined>): void;
    createAsset(photoType: PhotoType, extension: string, options?: CreateOptions): Promise<string>;
    getAlbums(type: AlbumType, subtype: AlbumSubtype, options: FetchOptions,
       callback: AsyncCallback<FetchResult<Album> | undefined>): void;
    getAlbums(type: AlbumType, subtype: AlbumSubtype, callback: AsyncCallback<FetchResult<Album> | undefined>): void;
    getAlbums(type: AlbumType, subtype: AlbumSubtype, options?: FetchOptions): Promise<FetchResult<Album>>;
    registerChange(uri: string, forChildUris: boolean, callback: Callback<ChangeData>): void;
    getHiddenAlbums(mode: HiddenPhotosDisplayMode, options: FetchOptions,
      callback: AsyncCallback<FetchResult<Album>| undefined>): void;
    getHiddenAlbums(mode: HiddenPhotosDisplayMode, callback: AsyncCallback<FetchResult<Album>| undefined>): void;
    getHiddenAlbums(mode: HiddenPhotosDisplayMode, options?: FetchOptions): Promise<FetchResult<Album>>;
    getDataAnalysisProgress(analysisType: AnalysisType): Promise<string>;
    getSharedPhotoAssets(options: FetchOptions): Array<SharedPhotoAsset>;
    unRegisterChange(uri: string, callback?: Callback<ChangeData>): void;
    createAssetsForApp(bundleName: string, appName: string, tokenId: long,
      photoCreationConfigs: Array<PhotoCreationConfig>): Promise<Array<string>>;
    createAssetsForAppWithMode(bundleName: string, appName: string, appId: string, tokenId: long,
      authorizationMode: AuthorizationMode, photoCreationConfigs: Array<PhotoCreationConfig>): Promise<Array<string>>;
    release(callback: AsyncCallback<void>): void;
    release(): Promise<void>;
    saveFormInfo(info: FormInfo, callback: AsyncCallback<void>): void;
    saveFormInfo(info: FormInfo): Promise<void>;
    saveGalleryFormInfo(info: GalleryFormInfo): Promise<void>;
    applyChanges(mediaChangeRequest: MediaChangeRequest): Promise<void>;
    getIndexConstructProgress(): Promise<string>;
    grantPhotoUrisPermission(tokenId: long, uriList: Array<string>, photoPermissionType: PhotoPermissionType,
      hideSensitiveType: HideSensitiveType): Promise<int>;
    grantPhotoUriPermission(tokenId: long, uri: string, photoPermissionType: PhotoPermissionType,
      hideSensitiveType: HideSensitiveType): Promise<int>;
    cancelPhotoUriPermission(tokenId: long, uri: string,
      photoPermissionType: PhotoPermissionType): Promise<int>;
    getPhotoIndex(photoUri: string, albumUri: string, options: FetchOptions,
        callback: AsyncCallback<int | undefined>): void;
    getPhotoIndex(photoUri: string, albumUri: string, options: FetchOptions): Promise<int>;
    removeFormInfo(info: FormInfo, callback: AsyncCallback<void>): void;
    removeFormInfo(info: FormInfo): Promise<void>;
    removeGalleryFormInfo(info: GalleryFormInfo): Promise<void>;
    getSupportedPhotoFormats(photoType: PhotoType): Promise<Array<string>>;
    startAssetAnalysis(type: AnalysisType, assetUris?: Array<string>): Promise<int>;
    createAssetsForAppWithAlbum(source: PhotoCreationSource, albumUri: string, isAuthorized: boolean,
      photoCreationConfigs: Array<PhotoCreationConfig>): Promise<Array<string>>;
    getAlbumsByIds(albumIds: Array<int>): Promise<Map<int, Album>>;
    updateGalleryFormInfo(info: GalleryFormInfo): Promise<void>;
    onSinglePhotoAlbumChange(album: Album, callback: Callback<AlbumChangeInfos>): void;
    offSinglePhotoAlbumChange(album?: Album, callback?: Callback<AlbumChangeInfos>): void;
    onSinglePhotoChange(asset: PhotoAsset, callback: Callback<PhotoAssetChangeInfos>): void;
    offSinglePhotoChange(asset?: PhotoAsset, callback?: Callback<PhotoAssetChangeInfos>): void;
    onPhotoChange(callback: Callback<PhotoAssetChangeInfos>): void;
    offPhotoChange(callback?: Callback<PhotoAssetChangeInfos>): void;
    onHiddenPhotoChange(callback: Callback<PhotoAssetChangeInfos>): void;
    offHiddenPhotoChange(callback?: Callback<PhotoAssetChangeInfos>): void;
    onTrashedPhotoChange(callback: Callback<PhotoAssetChangeInfos>): void;
    offTrashedPhotoChange(callback?: Callback<PhotoAssetChangeInfos>): void;
    onPhotoAlbumChange(callback: Callback<AlbumChangeInfos>): void;
    offPhotoAlbumChange(callback?: Callback<AlbumChangeInfos>): void;
    onHiddenAlbumChange(callback: Callback<AlbumChangeInfos>): void;
    offHiddenAlbumChange(callback?: Callback<AlbumChangeInfos>): void;
    onTrashedAlbumChange(callback: Callback<AlbumChangeInfos>): void;
    offTrashedAlbumChange(callback?: Callback<AlbumChangeInfos>): void;
    getPhotoAlbums(options?: FetchOptions): Promise<FetchResult<Album>>;
    getPhotoAlbumOrder(orderStyle: int, options?: FetchOptions): Promise<FetchResult<AlbumOrder>>;
    setPhotoAlbumOrder(orderStyle: int, albumOrders: Array<AlbumOrder>): Promise<void>;
  }

  interface AppInfo {
    bundleName: string;
    appName: string;
    appId: string;
    tokenId: long;
  }

  interface GrantPhotoUriPermissionParam {
    tokenId: long;
    uri: string;
  }

  interface GrantPhotoUrisPermissionParam {
    tokenId: long;
    uriList: Array<string>;
  }

  class PhotoAccessHelperHandle implements PhotoAccessHelper {
    private nativeHandle: long = 0;

    constructor(nativePtr: long) {
      if (this.nativeHandle == 0) {
        this.nativeHandle = nativePtr;
      }
    }

    removeGalleryFormInfo(info: GalleryFormInfo): Promise<void> {
      return new Promise<void>((resolve: (v: undefined) => void, reject: (e: BusinessError<void>) => void): void => {
        let cb = (): MediaLibraryAniError | undefined => {
          try {
            this.removeGalleryFormInfoInner(info);
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

    removeFormInfo(info: FormInfo, callback: AsyncCallback<void>): void {
      let cb = (): MediaLibraryAniError | undefined => {
        try {
          this.PhotoAccessRemoveFormInfo(info);
          return undefined;
        } catch(e) {
          return e as MediaLibraryAniError;
        }
      }
      taskpool.execute(cb).then((ret: Any): void => {
        if (isNullish(ret)) {
          callback(businessError(0), undefined);
        } else {
          let err = ret as MediaLibraryAniError;
          callback(businessError(err.code as int, err.message), undefined);
        }
      });
    }

    removeFormInfo(info: FormInfo): Promise<void> {
      return new Promise<void>((resolve: (v: undefined) => void, reject: (e: BusinessError<void>) => void): void => {
        let cb = (): MediaLibraryAniError | undefined => {
          try {
            this.PhotoAccessRemoveFormInfo(info);
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

    getPhotoIndex(photoUri: string, albumUri: string, options: FetchOptions,
      callback: AsyncCallback<int | undefined>): void {
      let cb = (): MediaLibraryAniError | int => {
        let result: MediaLibraryAniError | int;
        try {
          result = this.getPhotoIndexInner(photoUri, albumUri, options);
        } catch(e) {
          result = e as MediaLibraryAniError;
        }
        return result;
      };
      taskpool.execute(cb).then((ret: Any): void => {
        if (ret instanceof MediaLibraryAniError) {
          let err = ret as MediaLibraryAniError;
          callback(businessError(err.code as int, err.message), undefined);
        } else {
          if (isNullish(ret)) {
            callback(businessError(-1, "Operation failed"), undefined);
          } else {
            callback(businessError(0), ret as int);
          }
        }
      });
    }

    getPhotoIndex(photoUri: string, albumUri: string, options: FetchOptions): Promise<int> {
      return new Promise<int>((resolve: (v: int) => void, reject: RejectString) => {
        let cb = (): int | MediaLibraryAniError => {
          try {
            return this.getPhotoIndexInner(photoUri, albumUri, options);
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
              resolve(ret as int);
            }
          }
        });
      });
    }

    getAssets(options: FetchOptions, callback: AsyncCallback<FetchResult<PhotoAsset> | undefined>): void {
      let cb = (): MediaLibraryAniError | FetchResult<PhotoAsset> => {
        let result: MediaLibraryAniError | FetchResult<PhotoAsset>;
        try {
          result = this.getAssetsInner(options);
        } catch(e) {
          result = e as MediaLibraryAniError;
        }
        return result;
      };
      taskpool.execute(cb).then((ret: Any): void => {
        if (ret instanceof MediaLibraryAniError) {
          let err = ret as MediaLibraryAniError;
          callback(businessError(err.code as int, err.message), undefined);
        } else {
          if (isNullish(ret)) {
            callback(businessError(-1, "Operation failed"), undefined);
          } else {
            callback(businessError(0), ret as FetchResult<PhotoAsset>);
          }
        }
      });
    }

    getAssets(options: FetchOptions): Promise<FetchResult<PhotoAsset>> {
      return new Promise<FetchResult<PhotoAsset>>(
        (resolve: (v: FetchResult<PhotoAsset>) => void, reject: RejectString) => {
        let blockQueue: containers.BlockingQueue<FetchResult<PhotoAsset> | MediaLibraryAniError> =
          new containers.ArrayBlockingQueue<FetchResult<PhotoAsset> | MediaLibraryAniError>(2);
        let cb = (): FetchResult<PhotoAsset> | MediaLibraryAniError => {
          let result: FetchResult<PhotoAsset> | MediaLibraryAniError;
          try {
            result = this.getAssetsInner(options);
          } catch(e) {
            result = e as MediaLibraryAniError;
          }
          blockQueue.push(result);
          return result;
        };

        taskpool.execute(cb).then((buffer: Any): void => {
          if (buffer instanceof MediaLibraryAniError) {
            let err = buffer as MediaLibraryAniError;
            reject(businessError<string>(err.code as int, err.message));
          } else {
            if (isNullish(buffer)) {
              reject(businessError<string>(-1, "Operation failed"));
            } else {
              resolve(buffer as FetchResult<PhotoAsset>);
            }
          }
        });
        blockQueue.pop();
      });
    }

    getBurstAssets(burstKey: string, options: FetchOptions): Promise<FetchResult<PhotoAsset>> {
      return new Promise<FetchResult<PhotoAsset>>((resolve: (v: FetchResult<PhotoAsset>) => void,
        reject: RejectString) => {
        let cb = (): FetchResult<PhotoAsset> | MediaLibraryAniError => {
          let result: FetchResult<PhotoAsset> | MediaLibraryAniError;
          try {
            result = this.getBurstAssetsInner(burstKey, options);
          } catch(e) {
            result = e as MediaLibraryAniError;
          }
          return result;
        };
        taskpool.execute(cb).then((ret: Any): void => {
          if (ret instanceof MediaLibraryAniError) {
            let err = ret as MediaLibraryAniError;
            reject(businessError<string>(err.code as int, err.message));
          } else {
            if (isNullish(ret)) {
              reject(businessError<string>(-1, "Operation failed"));
            } else {
              resolve(ret as FetchResult<PhotoAsset>);
            }
          }
        });
      });
    }

    createAsset(displayName: string, callback: AsyncCallback<PhotoAsset | undefined>): void {
      let cb = (): MediaLibraryAniError | PhotoAsset => {
        let result: MediaLibraryAniError | PhotoAsset;
        try {
          result = this.createAssetSystemInner(displayName);
        } catch(e) {
          result = e as MediaLibraryAniError;
        }
        return result;
      };
      taskpool.execute(cb).then((ret: Any): void => {
        if (ret instanceof MediaLibraryAniError) {
          let err = ret as MediaLibraryAniError;
          callback(businessError(err.code as int, err.message), undefined);
        } else {
          if (isNullish(ret)) {
            callback(businessError(-1, "Operation failed"), undefined);
          } else {
            callback(businessError(0), ret as PhotoAsset);
          }
        }
      });
    }

    createAsset(displayName: string): Promise<PhotoAsset> {
      return new Promise<PhotoAsset>((resolve: (v: PhotoAsset) => void, reject: RejectString) => {
        let cb = (): PhotoAsset | MediaLibraryAniError => {
          let result: PhotoAsset | MediaLibraryAniError;
          try {
            result = this.createAssetSystemInner(displayName);
          } catch(e) {
            result = e as MediaLibraryAniError;
          }
          return result;
        };
        taskpool.execute(cb).then((ret: Any): void => {
          if (ret instanceof MediaLibraryAniError) {
            let err = ret as MediaLibraryAniError;
            reject(businessError<string>(err.code as int, err.message));
          } else {
            if (isNullish(ret)) {
              reject(businessError<string>(-1, "Operation failed"));
            } else {
              resolve(ret as PhotoAsset);
            }
          }
        });
      });
    }

    createAsset(displayName: string, options: PhotoCreateOptions): Promise<PhotoAsset> {
      return new Promise<PhotoAsset>((resolve: (v: PhotoAsset) => void, reject: RejectString) => {
        let cb = (): PhotoAsset | MediaLibraryAniError => {
          let result: PhotoAsset | MediaLibraryAniError;
          try {
            result = this.createAssetSystemInner(displayName, options);
          } catch(e) {
            result = e as MediaLibraryAniError;
          }
          return result;
        };
        taskpool.execute(cb).then((ret: Any): void => {
          if (ret instanceof MediaLibraryAniError) {
            let err = ret as MediaLibraryAniError;
            reject(businessError<string>(err.code as int, err.message));
          } else {
            if (isNullish(ret)) {
              reject(businessError<string>(-1, "Operation failed"));
            } else {
              resolve(ret as PhotoAsset);
            }
          }
        });
      });
    }

    createAsset(displayName: string, options: PhotoCreateOptions,
      callback: AsyncCallback<PhotoAsset | undefined>): void {
      let cb = (): MediaLibraryAniError | PhotoAsset => {
        let result: MediaLibraryAniError | PhotoAsset;
        try {
          result = this.createAssetSystemInner(displayName, options);
        } catch(e) {
          result = e as MediaLibraryAniError;
        }
        return result;
      };
      taskpool.execute(cb).then((ret: Any): void => {
        if (ret instanceof MediaLibraryAniError) {
          let err = ret as MediaLibraryAniError;
          callback(businessError(err.code as int, err.message), undefined);
        } else {
          if (isNullish(ret)) {
            callback(businessError(-1, "Operation failed"), undefined);
          } else {
            callback(businessError(0), ret as PhotoAsset);
          }
        }
      });
    }

    createAsset(photoType: PhotoType, extension: string, options: CreateOptions,
      callback: AsyncCallback<string | undefined>): void {
      let cb = (): MediaLibraryAniError | string => {
        let result: MediaLibraryAniError | string;
        try {
          result = this.createAssetComponentInner(photoType, extension, options);
        } catch(e) {
          result = e as MediaLibraryAniError;
        }
        return result;
      };
      taskpool.execute(cb).then((ret: Any): void => {
        if (ret instanceof MediaLibraryAniError) {
          let err = ret as MediaLibraryAniError;
          callback(businessError(err.code as int, err.message), undefined);
        } else {
          if (isNullish(ret)) {
            callback(businessError(-1, "Operation failed"), undefined);
          } else {
            callback(businessError(0), ret as string);
          }
        }
      });
    }

    createAsset(photoType: PhotoType, extension: string, callback: AsyncCallback<string | undefined>): void {
      let cb = (): MediaLibraryAniError | string => {
        let result: MediaLibraryAniError | string;
        try {
          result = this.createAssetComponentInner(photoType, extension);
        } catch(e) {
          result = e as MediaLibraryAniError;
        }
        return result;
      };
      taskpool.execute(cb).then((ret: Any): void => {
        if (ret instanceof MediaLibraryAniError) {
          let err = ret as MediaLibraryAniError;
          callback(businessError(err.code as int, err.message), undefined);
        } else {
          if (isNullish(ret)) {
            callback(businessError(-1, "Operation failed"), undefined);
          } else {
            callback(businessError(0), ret as string);
          }
        }
      });
    }

    createAsset(photoType: PhotoType, extension: string, options?: CreateOptions): Promise<string> {
      return new Promise<string>((resolve: (v: string) => void, reject: RejectString) => {
        let cb = (): string | MediaLibraryAniError => {
          let result: string | MediaLibraryAniError;
          try {
            result = this.createAssetComponentInner(photoType, extension, options);
          } catch(e) {
            result = e as MediaLibraryAniError;
          }
          return result;
        };
        taskpool.execute(cb).then((ret: Any): void => {
          if (ret instanceof MediaLibraryAniError) {
            let err = ret as MediaLibraryAniError;
            reject(businessError<string>(err.code as int, err.message));
          } else {
            if (isNullish(ret)) {
              reject(businessError<string>(-1, "Operation failed"));
            } else {
              resolve(ret as string);
            }
          }
        });
      });
    }

    getAlbums(type: AlbumType, subtype: AlbumSubtype, options: FetchOptions,
      callback: AsyncCallback<FetchResult<Album> | undefined>): void {
      let cb = (): MediaLibraryAniError | FetchResult<Album> => {
        let result: MediaLibraryAniError | FetchResult<Album>;
        try {
          result = this.getAlbumsInner(type, subtype, options);
        } catch(e) {
          result = e as MediaLibraryAniError;
        }
        return result;
      };
      taskpool.execute(cb).then((ret: Any): void => {
        if (ret instanceof MediaLibraryAniError) {
          let err = ret as MediaLibraryAniError;
          callback(businessError(err.code as int, err.message), undefined);
        } else {
          if (isNullish(ret)) {
            callback(businessError(-1, "Operation failed"), undefined);
          } else {
            callback(businessError(0), ret as FetchResult<Album>);
          }
        }
      });
    }

    getAlbums(type: AlbumType, subtype: AlbumSubtype, callback: AsyncCallback<FetchResult<Album> | undefined>): void {
      let cb = (): MediaLibraryAniError | FetchResult<Album> => {
        let result: MediaLibraryAniError | FetchResult<Album>;
        try {
          result = this.getAlbumsInner(type, subtype);
        } catch(e) {
          result = e as MediaLibraryAniError;
        }
        return result;
      };
      taskpool.execute(cb).then((ret: Any): void => {
        if (ret instanceof MediaLibraryAniError) {
          let err = ret as MediaLibraryAniError;
          callback(businessError(err.code as int, err.message), undefined);
        } else {
          if (isNullish(ret)) {
            callback(businessError(-1, "Operation failed"), undefined);
          } else {
            callback(businessError(0), ret as FetchResult<Album>);
          }
        }
      });
    }

    getAlbums(type: AlbumType, subtype: AlbumSubtype, options?: FetchOptions): Promise<FetchResult<Album>> {
      return new Promise<FetchResult<Album>>((resolve: (v: FetchResult<Album>) => void,
        reject: RejectString) => {
          let cb = (): FetchResult<Album> | MediaLibraryAniError => {
            let result: FetchResult<Album> | MediaLibraryAniError;
            try {
              result = this.getAlbumsInner(type, subtype, options);
            } catch(e) {
              result = e as MediaLibraryAniError;
            }
            return result;
          };
          taskpool.execute(cb).then((ret: Any): void => {
            if (ret instanceof MediaLibraryAniError) {
              let err = ret as MediaLibraryAniError;
              reject(businessError<string>(err.code as int, err.message));
            } else {
              if (isNullish(ret)) {
                reject(businessError<string>(-1, "Operation failed"));
              } else {
                resolve(ret as FetchResult<Album>);
              }
            }
          });
      });
    }

    getHiddenAlbums(mode: HiddenPhotosDisplayMode, options: FetchOptions,
      callback: AsyncCallback<FetchResult<Album>| undefined>): void {
      let cb = ():MediaLibraryAniError | FetchResult<Album> => {
        let result: MediaLibraryAniError | FetchResult<Album>;
        try {
          result =  this.getHiddenAlbumsInner(mode, options);
        } catch(e) {
          result =  e as MediaLibraryAniError;
        }
        return result;
      };
      taskpool.execute(cb).then((ret: Any): void => {
        if (ret instanceof MediaLibraryAniError) {
        let err = ret as MediaLibraryAniError;
        callback(businessError(err.code as int, err.message), undefined);
        } else {
          if (isNullish(ret)) {
            callback(businessError(-1, "Operation failed"), undefined);
          } else {
            callback(businessError(0), ret as FetchResult<Album>);
          }
        }
      });
    }

    getHiddenAlbums(mode: HiddenPhotosDisplayMode, callback: AsyncCallback<FetchResult<Album>| undefined>): void {
      let cb = ():MediaLibraryAniError |FetchResult<Album> => {
        let result: MediaLibraryAniError | FetchResult<Album>;
        try {
          result =  this.getHiddenAlbumsInner(mode);
        } catch(e) {
          result =   e as MediaLibraryAniError;
        }
        return result;
      };
      taskpool.execute(cb).then((ret: Any): void => {
        if (ret instanceof MediaLibraryAniError) {
          let err = ret as MediaLibraryAniError;
          callback(businessError(err.code as int, err.message), undefined);
        } else {
          if (isNullish(ret)) {
            callback(businessError(-1, "Operation failed"), undefined);
          } else {
            callback(businessError(0), ret as FetchResult<Album>);
          }
        }
      });
    }

    getHiddenAlbums(mode: HiddenPhotosDisplayMode, options?: FetchOptions): Promise<FetchResult<Album>> {
      return new Promise<FetchResult<Album>>((resolve: (v: FetchResult<Album>) => void,
        reject: RejectString) => {
        let cb = (): FetchResult<Album> | MediaLibraryAniError => {
          let result: MediaLibraryAniError | FetchResult<Album>;
          try {
            result =  this.getHiddenAlbumsInner(mode, options);
          } catch(e) {
            result =  e as MediaLibraryAniError;
          }
          return result;
        };
        taskpool.execute(cb).then((ret: Any): void => {
          if (ret instanceof MediaLibraryAniError) {
            let err = ret as MediaLibraryAniError;
            reject(businessError<string>(err.code as int, err.message));
          } else {
            if (isNullish(ret)) {
              reject(businessError<string>(-1, "Operation failed"));
            } else {
              resolve(ret as FetchResult<Album>);
            }
          }
        });
      });
    }

    getDataAnalysisProgress(analysisType: AnalysisType): Promise<string> {
      return new Promise<string>((resolve: (v: string) => void, reject: RejectString) => {
        let cb = (): string | MediaLibraryAniError => {
          let result: string | MediaLibraryAniError;
          try {
            result = this.getDataAnalysisProgressInner(analysisType);
          } catch(e) {
            result = e as MediaLibraryAniError;
          }
          return result;
        };
        taskpool.execute(cb).then((ret: Any): void => {
          if (ret instanceof MediaLibraryAniError) {
            let err = ret as MediaLibraryAniError;
            reject(businessError<string>(err.code as int, err.message));
          } else {
            if (isNullish(ret)) {
              reject(businessError<string>(-1, "Operation failed"));
            } else {
              resolve(ret as string);
            }
          }
        });
      });
    }

    getAlbumsByIds(albumIds: Array<int>): Promise<Map<int, Album>> {
      return new Promise<Map<int, Album>>((resolve: (v: Map<int, Album>) => void,
        reject: RejectString) => {
        let cb = (): Map<int, Album> | MediaLibraryAniError => {
          let result: Map<int, Album> | MediaLibraryAniError;
          try {
            result = this.getAlbumsByIdsInner(albumIds);
          } catch(e) {
            result = e as MediaLibraryAniError;
          }
          return result;
        };
        taskpool.execute(cb).then((ret: Any): void => {
          if (ret instanceof MediaLibraryAniError) {
            let err = ret as MediaLibraryAniError;
            reject(businessError<string>(err.code as int, err.message));
          } else {
            if (isNullish(ret)) {
              reject(businessError<string>(-1, "Operation failed"));
            } else {
              resolve(ret as Map<int, Album>);
            }
          }
        });
      });
    }

    createAssetsForApp(bundleName: string, appName: string, tokenId: long,
      photoCreationConfigs: Array<PhotoCreationConfig>): Promise<Array<string>> {
      let appInfo: AppInfo = {
        bundleName: bundleName,
        appName: appName,
        appId: "",
        tokenId: tokenId
      };
      return new Promise<Array<string>>((resolve: (v: Array<string>) => void, reject: RejectString) => {
        let cb = (): Array<string> | MediaLibraryAniError => {
          let result: Array<string> | MediaLibraryAniError;
          try {
            result = this.createAssetsForAppInner(appInfo, photoCreationConfigs);
          } catch(e) {
            result = e as MediaLibraryAniError;
          }
          return result;
        };
        taskpool.execute(cb).then((ret: Any): void => {
          if (ret instanceof MediaLibraryAniError) {
            let err = ret as MediaLibraryAniError;
            reject(businessError<string>(err.code as int, err.message));
          } else {
            if (isNullish(ret)) {
              reject(businessError<string>(-1, "Operation failed"));
            } else {
              resolve(ret as Array<string>);
            }
          }
        });
      });
    }

    createAssetsForAppWithAlbum(source: PhotoCreationSource, albumUri: string, isAuthorized: boolean,
      photoCreationConfigs: Array<PhotoCreationConfig>): Promise<Array<string>> {
      return new Promise<Array<string>>((resolve: (v: Array<string>) => void, reject: RejectString) => {
        let cb = (): Array<string> | MediaLibraryAniError => {
          let result: Array<string> | MediaLibraryAniError;
          try {
            result = this.createAssetsForAppWithAlbumInner(source, albumUri, isAuthorized,
              photoCreationConfigs);
          } catch (e) {
            result = e as MediaLibraryAniError;
          }
          return result;
        };

        taskpool.execute(cb).then((ret: Any): void => {
          if (ret instanceof MediaLibraryAniError) {
            let err = ret as MediaLibraryAniError;
            reject(businessError<string>(err.code as int, err.message));
          } else {
            if (isNullish(ret)) {
              reject(businessError<string>(-1, "Operation failed"));
            } else {
              resolve(ret as Array<string>);
            }
          }
        });
      });
    }

    createAssetsForAppWithMode(bundleName: string, appName: string, appId: string, tokenId: long,
      authorizationMode: AuthorizationMode, photoCreationConfigs: Array<PhotoCreationConfig>): Promise<Array<string>> {
      let appInfo: AppInfo = {
        bundleName: bundleName,
        appName: appName,
        appId: appId,
        tokenId: tokenId
      };
      return new Promise<Array<string>>((resolve: (v: Array<string>) => void, reject: RejectString) => {
        let cb = (): Array<string> | MediaLibraryAniError => {
          let result: Array<string> | MediaLibraryAniError;
          try {
            result = this.createAssetsForAppWithModeInner(appInfo, authorizationMode, photoCreationConfigs);
          } catch(e) {
            result = e as MediaLibraryAniError;
          }
          return result;
        };
        taskpool.execute(cb).then((ret: Any): void => {
          if (ret instanceof MediaLibraryAniError) {
            let err = ret as MediaLibraryAniError;
            reject(businessError<string>(err.code as int, err.message));
          } else {
            if (isNullish(ret)) {
              reject(businessError<string>(-1, "Operation failed"));
            } else {
              resolve(ret as Array<string>);
            }
          }
        });
      });
    }

    release(callback: AsyncCallback<void>): void {
      let cb = (): MediaLibraryAniError | undefined => {
        try {
          this.releaseInner();
          return undefined;
        } catch(e) {
          return e as MediaLibraryAniError;
        }
      }
      taskpool.execute(cb).then((ret: Any): void => {
        if (isNullish(ret)) {
          callback(businessError(0), undefined);
        } else {
          let err = ret as MediaLibraryAniError;
        }
      });
    }

    release(): Promise<void> {
      return new Promise<void>((resolve: (v: undefined) => void, reject: (e: BusinessError<void>) => void): void => {
        let cb = (): MediaLibraryAniError | undefined => {
          try {
            this.releaseInner();
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

    saveFormInfo(info: FormInfo, callback: AsyncCallback<void>): void {
      let cb = (): MediaLibraryAniError | undefined => {
        try {
          this.saveFormInfoInner(info);
          return undefined;
        } catch(e) {
          return e as MediaLibraryAniError;
        }
      }
      taskpool.execute(cb).then((ret: Any): void => {
        if (isNullish(ret)) {
          callback(businessError(0), undefined);
        } else {
          let err = ret as MediaLibraryAniError;
          callback(businessError(err.code as int, err.message), undefined);
        }
      });
    }

    saveFormInfo(info: FormInfo): Promise<void> {
      return new Promise<void>((resolve: (v: undefined) => void, reject: (e: BusinessError<void>) => void): void => {
        let cb = (): MediaLibraryAniError | undefined => {
          try {
            this.saveFormInfoInner(info);
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

    updateGalleryFormInfo(info: GalleryFormInfo): Promise<void> {
      return new Promise<void>((resolve: (v: undefined) => void, reject: (e: BusinessError<void>) => void): void => {
        let cb = (): MediaLibraryAniError | undefined => {
          try {
            this.updateGalleryFormInfoInner(info);
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

    saveGalleryFormInfo(info: GalleryFormInfo): Promise<void> {
      return new Promise<void>((resolve: (v: undefined) => void, reject: (e: BusinessError<void>) => void): void => {
        let cb = (): MediaLibraryAniError | undefined => {
          try {
            this.saveGalleryFormInfoInner(info);
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
      })
    }

    applyChanges(mediaChangeRequest: MediaChangeRequest): Promise<void> {
      return new Promise<void>((resolve: (v: undefined) => void, reject: (e: BusinessError<void>) => void): void => {
        let cb = (): MediaLibraryAniError | undefined => {
          try {
            this.applyChangesInner(mediaChangeRequest);
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

    getIndexConstructProgress(): Promise<string> {
      return new Promise<string>((resolve: (v: string) => void, reject: RejectString) => {
        let cb = (): string | MediaLibraryAniError => {
          let result: string | MediaLibraryAniError;
          try {
            result = this.getIndexConstructProgressInner();
          } catch(e) {
            result = e as MediaLibraryAniError;
          }
          return result;
        };
        taskpool.execute(cb).then((ret: Any): void => {
          if (ret instanceof MediaLibraryAniError) {
            let err = ret as MediaLibraryAniError;
            reject(businessError<string>(err.code as int, err.message));
          } else {
            if (isNullish(ret)) {
              reject(businessError<string>(-1, "Operation failed"));
            } else {
              resolve(ret as string);
            }
          }
        });
      });
    }

    grantPhotoUrisPermission(tokenId: long, uriList: Array<string>, photoPermissionType: PhotoPermissionType,
      hideSensitiveType: HideSensitiveType): Promise<int> {
      let param: GrantPhotoUrisPermissionParam = {
        tokenId: tokenId,
        uriList: uriList
      };
      return new Promise<int>((resolve: (v: int) => void, reject: RejectString) => {
        let cb = (): int | MediaLibraryAniError => {
          let result: int | MediaLibraryAniError;
          try {
            result = this.grantPhotoUrisPermissionInner(param, photoPermissionType, hideSensitiveType);
          } catch(e) {
            result = e as MediaLibraryAniError;
          }
          return result;
        };
        taskpool.execute(cb).then((ret: Any): void => {
          if (ret instanceof MediaLibraryAniError) {
            let err = ret as MediaLibraryAniError;
            reject(businessError<string>(err.code as int, err.message));
          } else {
            if (isNullish(ret)) {
              reject(businessError<string>(-1, "Operation failed"));
            } else {
              resolve(ret as int);
            }
          }
        });
      });
    }

    grantPhotoUriPermission(tokenId: long, uri: string, photoPermissionType: PhotoPermissionType,
      hideSensitiveType: HideSensitiveType): Promise<int> {
      let param: GrantPhotoUriPermissionParam = {
        tokenId: tokenId,
        uri: uri
      };
      return new Promise<int>((resolve: (v: int) => void, reject: RejectString) => {
        let cb = (): int | MediaLibraryAniError => {
          let result: int | MediaLibraryAniError;
          try {
            result = this.grantPhotoUriPermissionInner(param, photoPermissionType, hideSensitiveType);
          } catch(e) {
            result = e as MediaLibraryAniError;
          }
          return result;
        };
        taskpool.execute(cb).then((ret: Any): void => {
          if (ret instanceof MediaLibraryAniError) {
            let err = ret as MediaLibraryAniError;
            reject(businessError<string>(err.code as int, err.message));
          } else {
            if (isNullish(ret)) {
              reject(businessError<string>(-1, "Operation failed"));
            } else {
              resolve(ret as int);
            }
          }
        });
      });
    }

    cancelPhotoUriPermission(tokenId: long, uri: string,
      photoPermissionType: PhotoPermissionType): Promise<int> {
      return new Promise<int>((resolve: (v: int) => void, reject: RejectString) => {
        let cb = (): int | MediaLibraryAniError => {
          let result: int | MediaLibraryAniError;
          try {
            result = this.cancelPhotoUriPermissionInner(tokenId, uri, photoPermissionType);
          } catch(e) {
            result = e as MediaLibraryAniError;
          }
          return result;
        };
        taskpool.execute(cb).then((ret: Any): void => {
          if (ret instanceof MediaLibraryAniError) {
            let err = ret as MediaLibraryAniError;
            reject(businessError<string>(err.code as int, err.message));
          } else {
            if (isNullish(ret)) {
              reject(businessError<string>(-1, "Operation failed"));
            } else {
              resolve(ret as int);
            }
          }
        });
      });
    }

    getSupportedPhotoFormats(photoType: PhotoType): Promise<Array<string>> {
      return new Promise<Array<string>>((resolve: (v: Array<string>) => void, reject: RejectString) => {
        let cb = (): Array<string> | MediaLibraryAniError => {
          let result: Array<string> | MediaLibraryAniError;
          try {
            result = this.getSupportedPhotoFormatsInner(photoType);
          } catch(e) {
            result = e as MediaLibraryAniError;
          }
          return result;
        };
        taskpool.execute(cb).then((ret: Any): void => {
          if (ret instanceof MediaLibraryAniError) {
            let err = ret as MediaLibraryAniError;
            reject(businessError<string>(err.code as int, err.message));
          } else {
            if (isNullish(ret)) {
              reject(businessError<string>(-1, "Operation failed"));
            } else {
              resolve(ret as Array<string>);
            }
          }
        });
      });
    }

    startAssetAnalysis(type: AnalysisType, assetUris?: Array<string>): Promise<int> {
      return new Promise<int>((resolve: (v: int) => void, reject: RejectString) => {
        let cb = (): int | MediaLibraryAniError => {
          let result: int | MediaLibraryAniError;
          try {
            result = this.startAssetAnalysisInner(type, assetUris);
          } catch(e) {
            result = e as MediaLibraryAniError;
          }
          return result;
        };
        taskpool.execute(cb).then((ret: Any): void => {
          if (ret instanceof MediaLibraryAniError) {
            let err = ret as MediaLibraryAniError;
            reject(businessError<string>(err.code as int, err.message));
          } else {
            if (isNullish(ret)) {
              reject(businessError<string>(-1, "Operation failed"));
            } else {
              resolve(ret as int);
            }
          }
        });
      });
    }

    onSinglePhotoAlbumChange(album: Album, callback: Callback<AlbumChangeInfos>): void {
      let cb = (): MediaLibraryAniError | undefined => {
        try {
          this.onSinglePhotoAlbumChangeInner(album, callback);
          return undefined;
        } catch(e) {
          return e as MediaLibraryAniError;
        }
      }
    }
	
	  offSinglePhotoAlbumChange(album?: Album, callback?: Callback<AlbumChangeInfos>): void {
      let cb = (): MediaLibraryAniError | undefined => {
        try {
          this.offSinglePhotoAlbumChangeInner(album, callback);
          return undefined;
        } catch(e) {
          return e as MediaLibraryAniError;
        }
      }
    }
	
	  onSinglePhotoChange(asset: PhotoAsset, callback: Callback<PhotoAssetChangeInfos>): void {
      let cb = (): MediaLibraryAniError | undefined => {
        try {
          this.onSinglePhotoChangeInner(asset, callback);
          return undefined;
        } catch(e) {
          return e as MediaLibraryAniError;
        }
      }
    }

	  offSinglePhotoChange(asset?: PhotoAsset, callback?: Callback<PhotoAssetChangeInfos>): void {
      let cb = (): MediaLibraryAniError | undefined => {
        try {
          this.offSinglePhotoChangeInner(asset, callback);
          return undefined;
        } catch(e) {
          return e as MediaLibraryAniError;
        }
      }
    }

    getPhotoAlbums(options?: FetchOptions): Promise<FetchResult<Album>> {
      return new Promise<FetchResult<Album>>((resolve: (v: FetchResult<Album>) => void, reject: RejectString) => {
        let cb = (): FetchResult<Album> | MediaLibraryAniError => {
          let result: FetchResult<Album> | MediaLibraryAniError;
          try {
            result = this.getPhotoAlbumsInner(options);
          } catch(e) {
            result = e as MediaLibraryAniError;
          }
          return result;
        };
        taskpool.execute(cb).then((ret: Any): void => {
          if (ret instanceof MediaLibraryAniError) {
            let err = ret as MediaLibraryAniError;
            reject(businessError<string>(err.code as int, err.message));
          } else {
            if (isNullish(ret)) {
              reject(businessError<string>(-1, "Operation failed"));
            } else {
              resolve(ret as FetchResult<Album>);
            }
          }
        });
      });
    }

    getPhotoAlbumOrder(orderStyle: int, options?: FetchOptions): Promise<FetchResult<AlbumOrder>> {
      return new Promise<FetchResult<AlbumOrder>>((resolve: (v: FetchResult<AlbumOrder>) => void,
        reject: RejectString) => {
        let cb = (): FetchResult<AlbumOrder> | MediaLibraryAniError => {
          let result: FetchResult<AlbumOrder> | MediaLibraryAniError;
          try {
            result = this.getPhotoAlbumsOrderInner(orderStyle, options);
          } catch(e) {
            result = e as MediaLibraryAniError;
          }
          return result;
        };
        taskpool.execute(cb).then((ret: Any): void => {
          if (ret instanceof MediaLibraryAniError) {
            let err = ret as MediaLibraryAniError;
            reject(businessError<string>(err.code as int, err.message));
          } else {
            if (isNullish(ret)) {
              reject(businessError<string>(-1, "Operation failed"));
            } else {
              resolve(ret as FetchResult<AlbumOrder>);
            }
          }
        });
      });
    }

    setPhotoAlbumOrder(orderStyle: int, albumOrders: Array<AlbumOrder>): Promise<void> {
      return new Promise<void>((resolve: (v: undefined) => void, reject: (e: BusinessError<void>) => void): void => {
        let cb = (): MediaLibraryAniError | undefined => {
          try {
            this.setPhotoAlbumsOrderInner(orderStyle, albumOrders);
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

    native startThumbnailCreationTask(predicate: dataSharePredicates.DataSharePredicates,
      callback: AsyncCallback<void>): int;
    native startCreateThumbnailTask(predicate: dataSharePredicates.DataSharePredicates): int;
    native stopThumbnailCreationTask(taskId: int): void;
    native registerChange(uri: string, forChildUris: boolean, callback: Callback<ChangeData>): void;
    native unRegisterChange(uri: string, callback?: Callback<ChangeData>): void;
    native getAssetsSync(options: FetchOptions): Array<PhotoAsset>;
    native getFileAssetsInfo(options: FetchOptions): Array<FileAssetInfo>;
    native getAssetsInner(options: FetchOptions): FetchResult<PhotoAsset>;
    native getBurstAssetsInner(burstKey: string, options: FetchOptions): FetchResult<PhotoAsset>;
    native createAssetSystemInner(displayName: string, options?: PhotoCreateOptions): PhotoAsset;
    native createAssetComponentInner(photoType: PhotoType, extension: string, options?: CreateOptions): string;
    native getAlbumsInner(type: AlbumType, subtype: AlbumSubtype, options?: FetchOptions): FetchResult<Album>;
    native getHiddenAlbumsInner(mode: HiddenPhotosDisplayMode, options?: FetchOptions): FetchResult<Album>;
    native getSharedPhotoAssets(options: FetchOptions): Array<SharedPhotoAsset>;
    native getDataAnalysisProgressInner(analysisType: AnalysisType): string;
    native getAlbumsByIdsInner(albumIds: Array<int>): Map<int, Album>;
    native createAssetsForAppInner(appInfo: AppInfo, photoCreationConfigs: Array<PhotoCreationConfig>): Array<string>;
    native createAssetsForAppWithModeInner(appInfo: AppInfo, authorizationMode: AuthorizationMode,
      photoCreationConfigs: Array<PhotoCreationConfig>): Array<string>;
    native createAssetsForAppWithAlbumInner(source: PhotoCreationSource, albumUri: string, isAuthorized: boolean,
      photoCreationConfigs: Array<PhotoCreationConfig>): Array<string>;
    native releaseInner(): void;
    native saveFormInfoInner(info: FormInfo): void;
    native saveGalleryFormInfoInner(info: GalleryFormInfo): void;
    native applyChangesInner(mediaChangeRequest: MediaChangeRequest): void;
    native getIndexConstructProgressInner(): string;
    native grantPhotoUrisPermissionInner(param: GrantPhotoUrisPermissionParam,
      photoPermissionType: PhotoPermissionType, hideSensitiveType: HideSensitiveType): int;
    native grantPhotoUriPermissionInner(param: GrantPhotoUriPermissionParam, photoPermissionType: PhotoPermissionType,
      hideSensitiveType: HideSensitiveType): int;
    native cancelPhotoUriPermissionInner(tokenId: long, uri: string,
      photoPermissionType: PhotoPermissionType): int;
    native getPhotoIndexInner(photoUri: string, albumUri: string, options: FetchOptions): int;
    native PhotoAccessRemoveFormInfo(info: FormInfo): void;
    native removeGalleryFormInfoInner(info: GalleryFormInfo): void;
    native updateGalleryFormInfoInner(info: GalleryFormInfo): void;
    native getSupportedPhotoFormatsInner(photoType: PhotoType): Array<string>;
    native startAssetAnalysisInner(type: AnalysisType, assetUris?: Array<string>): int;
    native onSinglePhotoAlbumChangeInner(album: Album, callback: Callback<AlbumChangeInfos>): void;
    native offSinglePhotoAlbumChangeInner(album?: Album, callback?: Callback<AlbumChangeInfos>): void;
    native onSinglePhotoChangeInner(asset: PhotoAsset, callback: Callback<PhotoAssetChangeInfos>): void;
    native offSinglePhotoChangeInner(asset?: PhotoAsset, callback?: Callback<PhotoAssetChangeInfos>): void;
    native onPhotoChange(callback: Callback<PhotoAssetChangeInfos>): void;
    native offPhotoChange(callback?: Callback<PhotoAssetChangeInfos>): void;
    native onHiddenPhotoChange(callback: Callback<PhotoAssetChangeInfos>): void;
    native offHiddenPhotoChange(callback?: Callback<PhotoAssetChangeInfos>): void;
    native onTrashedPhotoChange(callback: Callback<PhotoAssetChangeInfos>): void;
    native offTrashedPhotoChange(callback?: Callback<PhotoAssetChangeInfos>): void;
    native onPhotoAlbumChange(callback: Callback<AlbumChangeInfos>): void;
    native offPhotoAlbumChange(callback?: Callback<AlbumChangeInfos>): void;
    native onHiddenAlbumChange(callback: Callback<AlbumChangeInfos>): void;
    native offHiddenAlbumChange(callback?: Callback<AlbumChangeInfos>): void;
    native onTrashedAlbumChange(callback: Callback<AlbumChangeInfos>): void;
    native offTrashedAlbumChange(callback?: Callback<AlbumChangeInfos>): void;
    native getPhotoAlbumsInner(options?: FetchOptions): FetchResult<Album>;
    native getPhotoAlbumsOrderInner(orderStyle: int, options?: FetchOptions): FetchResult<AlbumOrder>;
    native setPhotoAlbumsOrderInner(orderStyle: int, albumOrders: Array<AlbumOrder>): void;
  }

  export interface GalleryFormInfo {
    formId: string;
    assetUris?: Array<string>;
  }

  export interface FormInfo {
    formId: string;
    uri: string;
  }

  export enum NotifyType {
    NOTIFY_ADD,
    NOTIFY_UPDATE,
    NOTIFY_REMOVE,
    NOTIFY_ALBUM_ADD_ASSET,
    NOTIFY_ALBUM_REMOVE_ASSET
  }

  export enum DefaultChangeUri {
    DEFAULT_PHOTO_URI = 'file://media/Photo',
    DEFAULT_ALBUM_URI = 'file://media/PhotoAlbum',
    DEFAULT_HIDDEN_ALBUM_URI = 'file://media/HiddenAlbum'
  }

  export interface ChangeData {
    type: NotifyType;
    uris: Array<string>;
    extraUris: Array<string>;
    sharedPhotoAssets: Array<SharedPhotoAsset>;
    sharedAlbumAssets: Array<SharedAlbumAsset>;
    sharedExtraPhotoAssets: Array<SharedPhotoAsset>;
  }

  class ChangeDataHandle implements ChangeData {
    constructor() {}
    type: NotifyType;
    uris: Array<string> = new Array<string>();
    extraUris: Array<string> = new Array<string>();
    sharedPhotoAssets: Array<SharedPhotoAsset> = new Array<SharedPhotoAsset>();
    sharedAlbumAssets: Array<SharedAlbumAsset> = new Array<SharedAlbumAsset>();
    sharedExtraPhotoAssets: Array<SharedPhotoAsset> = new Array<SharedPhotoAsset>();
  }

  export enum PhotoViewMIMETypes {
    IMAGE_TYPE = 'image/*',
    VIDEO_TYPE = 'video/*',
    IMAGE_VIDEO_TYPE = '*/*',
    MOVING_PHOTO_IMAGE_TYPE = 'image/movingPhoto'
  }

  export class BaseSelectOptions {
    MIMEType: PhotoViewMIMETypes | undefined;
    maxSelectNumber: int | undefined;
    isSearchSupported: boolean | undefined;
    isPhotoTakingSupported: boolean | undefined;
    recommendationOptions: RecommendationOptions | undefined;
    preselectedUris: Array<string> | undefined;
    isPreviewForSingleSelectionSupported: boolean | undefined;
  }

  export class PhotoSelectOptions extends BaseSelectOptions {
    isEditSupported: boolean | undefined;
    isOriginalSupported: boolean | undefined;
    subWindowName: string | undefined;
    themeColor: CustomColors | undefined;
    completeButtonText: CompleteButtonText | undefined;
  }

  export class RecommendationOptions {
    recommendationType: RecommendationType | undefined;
    textContextInfo: TextContextInfo | undefined;
  }

  export interface TextContextInfo {
    text?: string;
  }

  export class PhotoSelectResult {
    photoUris: Array<string> = new Array<string>();
    isOriginalPhoto: boolean = false;
  }

  export class PhotoViewPicker {
  }

  export class MediaAssetEditData {
    private _compatibleFormat: string;
    private _formatVersion: string;
    private _data: string;
    private nativeHandle: long = 0;

    private native nativeConstructor(compatibleFormat: string, formatVersion: string): void;
    private native compatibleFormatSetter(format: string): void;
    private native compatibleFormatGetter(): string;
    private native formatVersionSetter(version: string): void;
    private native formatVersionGetter(): string;
    private native dataSetter(data: string): void;
    private native dataGetter(): string;

    constructor(compatibleFormat: string, formatVersion: string) {
      this.nativeConstructor(compatibleFormat, formatVersion);
    }

    private create(context: long): void {
      if (this.nativeHandle == 0) {
        this.nativeHandle = context;
      }
    }

    set compatibleFormat(format: string) {
      this.compatibleFormatSetter(format);
    }

    get compatibleFormat(): string {
      return this.compatibleFormatGetter();
    }

    set formatVersion(version: string) {
      this.formatVersionSetter(version);
    }

    get formatVersion(): string {
      return this.formatVersionGetter();
    }

    set data(data: string) {
      this.dataSetter(data);
    }

    get data(): string {
      return this.dataGetter();
    }
  }

  export enum ResourceType {
    IMAGE_RESOURCE = 1,
    VIDEO_RESOURCE = 2,
    PHOTO_PROXY = 3,
    PRIVATE_MOVING_PHOTO_RESOURCE = 4,
    PRIVATE_MOVING_PHOTO_METADATA = 5
  }

  export enum ImageFileType {
    JPEG = 1,
    HEIF = 2
  }

  export enum MovingPhotoEffectMode {
    DEFAULT = 0,
    BOUNCE_PLAY = 1,
    LOOP_PLAY = 2,
    LONG_EXPOSURE = 3,
    MULTI_EXPOSURE = 4,
    CINEMA_GRAPH = 5,
    IMAGE_ONLY = 10
  }

  export enum VideoEnhancementType {
    QUALITY_ENHANCEMENT_LOCAL = 0,
    QUALITY_ENHANCEMENT_CLOUD = 1,
    QUALITY_ENHANCEMENT_LOCAL_AND_CLOUD = 2
  }

  export interface MediaChangeRequest {}

  export type AssetType = PhotoAsset | string;

  export class MediaAssetChangeRequest implements MediaChangeRequest {
    private nativeHandle: long = 0;

    constructor(asset: PhotoAsset) {
      this.nativeConstructor(asset);
    }

    native static createImageAssetRequest(context: Context, fileUri: string): MediaAssetChangeRequest;

    native static createVideoAssetRequest(context: Context, fileUri: string): MediaAssetChangeRequest;

    static createAssetRequest(context: Context, displayName: string,
      options?: PhotoCreateOptions): MediaAssetChangeRequest {
      return MediaAssetChangeRequest.createAssetRequestByPhotoCreateOptions(context, displayName, options);
    }

    static createAssetRequest(context: Context, photoType: PhotoType, extension: string,
      options?: CreateOptions): MediaAssetChangeRequest {
      return MediaAssetChangeRequest.createAssetRequestByCreateOptions(context, photoType, extension, options);
    }

    static deleteAssets(context: Context, assets: Array<AssetType> | Array<string>): Promise<void> {
      return new Promise<void>((resolve: (v: undefined) => void, reject: (e: BusinessError<void>) => void): void => {
        let cb = (): MediaLibraryAniError | undefined => {
          try {
            MediaAssetChangeRequest.deleteAssetsSync(context, assets);
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

    getWriteCacheHandler(): Promise<int> {
      return new Promise<int>((resolve: (v: int) => void, reject: RejectString) : void => {
        let cb = (): int | MediaLibraryAniError => {
          let result: int | MediaLibraryAniError;
          try {
            result = this.getWriteCacheHandlerInner();
          } catch(e) {
            result = e as MediaLibraryAniError;
          }
          return result;
        };
        taskpool.execute(cb).then((ret: Any): void => {
          if (ret instanceof MediaLibraryAniError) {
            let err = ret as MediaLibraryAniError;
            reject(businessError<string>(err.code as int, err.message));
          } else {
            if (isNullish(ret)) {
              reject(businessError<string>(-1, "Operation failed"));
            } else {
              resolve(ret as int);
            }
          }
        });
      });
    }

    native getAsset(): PhotoAsset;

    native setFavorite(favoriteState: boolean): void;
    native setHidden(hiddenState: boolean): void;
    native setUserComment(userComment: string): void;
    native setLocation(longitude: double, latitude: double): void;
    native setTitle(title: string): void;

    native setEditData(editData: MediaAssetEditData): void;

    native getWriteCacheHandlerInner(): int;

    native addResource(type: ResourceType, fileUri: string): void;

    native addResource(type: ResourceType, data: ArrayBuffer): void;

    native addResource(type: ResourceType, proxy: PhotoProxy): void;

    native setCameraShotKey(cameraShotKey: string): void;

    native saveCameraPhoto(): void;

    native saveCameraPhoto(imageFileType: ImageFileType): void;

    native discardCameraPhoto(): void;

    native setEffectMode(mode: MovingPhotoEffectMode): void;

    native setOrientation(orientation: int): void;

    native setVideoEnhancementAttr(videoEnhancementType: VideoEnhancementType, photoId: string): void;

    native setSupportedWatermarkType(watermarkType: WatermarkType): void;

    static deleteLocalAssetsPermanently(context: Context, assets: Array<PhotoAsset>): Promise<void> {
      return new Promise<void>((resolve: (v: undefined) => void, reject: (e: BusinessError<void>) => void): void => {
        let cb = (): MediaLibraryAniError | undefined => {
          try {
            MediaAssetChangeRequest.deleteLocalAssetsPermanentlySync(context, assets);
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

    static deleteLocalAssetsPermanentlyWithUri(context: Context, assetUris: Array<string>): Promise<void> {
      return new Promise<void>((resolve: (v: undefined) => void, reject: (e: BusinessError<void>) => void): void => {
        let cb = (): MediaLibraryAniError | undefined => {
          try {
            MediaAssetChangeRequest.deleteLocalAssetsPermanentlyWithUriSync(context, assetUris);
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

    private create(nativePointer: long): void {
      if (this.nativeHandle == 0) {
        this.nativeHandle = nativePointer;
      }
    }

    private native nativeConstructor(asset: PhotoAsset): void;
    private native static createAssetRequestByPhotoCreateOptions(context: Context, displayName: string,
      options?: PhotoCreateOptions): MediaAssetChangeRequest;
    private native static createAssetRequestByCreateOptions(context: Context, photoType: PhotoType, extension: string,
      options?: CreateOptions): MediaAssetChangeRequest;
    private native static deleteAssetsSync(context: Context, assets: Array<AssetType> | Array<string>): void;
    private native static deleteLocalAssetsPermanentlySync(context: Context, assets: Array<PhotoAsset>): void;
    private native static deleteLocalAssetsPermanentlyWithUriSync(context: Context, assetUris: Array<string>): void;
  }

  export class MediaAssetsChangeRequest implements MediaChangeRequest {
    private nativeHandle: long = 0;

    constructor(assets: Array<PhotoAsset>) {
      this.nativeConstructor(assets);
    }

    native setFavorite(favoriteState: boolean): void;

    native setHidden(hiddenState: boolean): void;

    native setUserComment(userComment: string): void;
    native setIsRecentShow(isRencentShow: boolean): void;

    private create(context: long): void {
      if (this.nativeHandle == 0) {
        this.nativeHandle = context;
      }
    }

    native nativeConstructor(assets: Array<PhotoAsset>): void;
  }

  export class MediaAlbumChangeRequest implements MediaChangeRequest {
    private nativeHandle: long = 0;

    constructor(album: Album) {
      this.nativeConstructor(album);
    }

    native static createAlbumRequest(context: Context, name: string): MediaAlbumChangeRequest;

    static deleteAlbums(context: Context, albums: Array<Album>): Promise<void> {
      return new Promise<void>((resolve: (v: undefined) => void, reject: RejectString) : void => {
        let cb = (): undefined | MediaLibraryAniError => {
          try {
            return MediaAlbumChangeRequest.deleteAlbumsSync(context, albums);
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
              resolve(undefined);
            } else {
              reject(businessError<string>(-1, 'Operation failed'));
            }
          }
        });
      });
    }

    static deleteAlbumsWithUri(context: Context, albumUris: Array<string>): Promise<void> {
      return new Promise<void>((resolve: (v: undefined) => void, reject: RejectString) : void => {
        let cb = (): undefined | MediaLibraryAniError => {
          try {
            return MediaAlbumChangeRequest.deleteAlbumsWithUriSync(context, albumUris);
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
              resolve(undefined);
            } else {
              reject(businessError<string>(-1, 'Operation failed'));
            }
          }
        });
      });
    }

    native getAlbum(): Album;

    native setCoverUri(coverUri: string): void;

    native setAlbumName(name: string): void;

    native addAssets(assets: Array<PhotoAsset>): void;

    native moveAssets(assets: Array<PhotoAsset>, targetAlbum: Album): void;

    native moveAssetsWithUri(assetUris: Array<string>, targetAlbum: Album): void;

    native recoverAssets(assets: Array<PhotoAsset>): void;

    native recoverAssetsWithUri(assetUris: Array<string>): void;

    native deleteAssets(assets: Array<PhotoAsset>): void;

    native deleteAssetsWithUri(assetUris: Array<string>): void;

    native setIsMe(): void;

    native setDisplayLevel(displayLevel: int): void;

    native dismissAssets(assets: Array<PhotoAsset>): void;

    native mergeAlbum(album: Album): void;

    native placeBefore(album: Album): void;

    native dismiss(): void;

    native resetCoverUri(): void;

    native removeAssets(assets: Array<PhotoAsset>): void;

    private create(context: long): void {
      if (this.nativeHandle == 0) {
        this.nativeHandle = context;
      }
    }

    native nativeConstructor(album: Album): void;
    native static deleteAlbumsSync(context: Context, albums: Array<Album>): void;
    native static deleteAlbumsWithUriSync(context: Context, albumUris: Array<string>): void;
  }

  export class MediaAnalysisAlbumChangeRequest extends MediaAlbumChangeRequest {
    private nativeHandle: long = 0;
    constructor(album: Album) {
      super(album);
    }
    private create(context: long): void {
      if (this.nativeHandle == 0) {
        this.nativeHandle = context;
      }
    }

    native setOrderPosition(assets: Array<PhotoAsset>, position: Array<int>): void;
  }

  export interface SharedPhotoAsset {
    fileId: int;
    uri: string;
    data: string;
    mediaType: PhotoType;
    displayName: string;
    size: long;
    dateAdded: long;
    dateModified: long;
    duration: int;
    width: int;
    height: int;
    dateTaken: long;
    orientation: int;
    isFavorite: boolean;
    title: string;
    position: PositionType;
    dateTrashed: long;
    hidden: boolean;
    userComment: string;
    cameraShotKey: string;
    dateYear: string;
    dateMonth: string;
    dateDay: string;
    pending: boolean;
    dateAddedMs: long;
    dateModifiedMs: long;
    dateTrashedMs: long;
    subtype: PhotoSubtype;
    movingPhotoEffectMode: MovingPhotoEffectMode;
    dynamicRangeType: DynamicRangeType;
    thumbnailReady: boolean;
    lcdSize: string;
    thmSize: string;
    thumbnailModifiedMs?: long;
    thumbnailVisible: ThumbnailVisibility;
  }

  class SharedPhotoAssetHandle implements SharedPhotoAsset {
    fileId: int;
    uri: string = "";
    data: string = "";
    mediaType: PhotoType;
    displayName: string = "";
    size: long;
    dateAdded: long;
    dateModified: long;
    duration: int;
    width: int;
    height: int;
    dateTaken: long;
    orientation: int;
    isFavorite: boolean;
    title: string = "";
    position: PositionType;
    dateTrashed: long;
    hidden: boolean;
    userComment: string = "";
    cameraShotKey: string = "";
    dateYear: string = "";
    dateMonth: string = "";
    dateDay: string = "";
    pending: boolean;
    dateAddedMs: long;
    dateModifiedMs: long;
    dateTrashedMs: long;
    subtype: PhotoSubtype;
    movingPhotoEffectMode: MovingPhotoEffectMode;
    dynamicRangeType: DynamicRangeType;
    thumbnailReady: boolean;
    lcdSize: string = "";
    thmSize: string = "";
    thumbnailModifiedMs?: long;
    thumbnailVisible: ThumbnailVisibility;
  }

  export interface SharedAlbumAsset {
    albumId: int;
    albumType: AlbumType;
    albumSubtype: AlbumSubtype;
    albumName: string;
    coverUri: string;
    count: int;
    imageCount: int;
    videoCount: int;
  }

  class SharedAlbumAssetHandle implements SharedAlbumAsset {
    albumId: int;
    albumType: AlbumType;
    albumSubtype: AlbumSubtype;
    albumName: string = "";
    coverUri: string = "";
    count: int;
    imageCount: int;
    videoCount: int;
    coverSharedPhotoAsset: SharedPhotoAsset;
  }

  export interface MovingPhoto {
    requestContent(imageFileUri: string, videoFileUri: string): Promise<void>;
    requestContent(resourceType: ResourceType, fileUri: string): Promise<void>;
    requestContent(resourceType: ResourceType): Promise<ArrayBuffer>;
    getUri(): string;
    isVideoReady(): Promise<boolean>;
  }

  export interface FileAssetInfo {
    file_id: int;
    uri: string;
    media_type: PhotoType;
    display_name: string;
    size: long;
    date_added: long;
    date_modified: long;
    duration: int;
    width: int;
    height: int;
    date_taken: long;
    orientation: int;
    is_favorite: boolean;
    title: string;
    position: PositionType;
    date_trashed: long;
    hidden: boolean;
    user_comment: string;
    camera_shot_key: string;
    date_year: string;
    date_month: string;
    date_day: string;
    pending: boolean;
    date_added_ms: long;
    date_modified_ms: long;
    date_trashed_ms: long;
    subtype: PhotoSubtype;
  }

  class FileAssetInfoHandle implements FileAssetInfo {
    file_id: int;
    uri: string = "";
    media_type: PhotoType;
    display_name: string = "";
    size: long;
    date_added: long;
    date_modified: long;
    duration: int;
    width: int;
    height: int;
    date_taken: long;
    orientation: int;
    is_favorite: boolean;
    title: string = "";
    position: PositionType;
    date_trashed: long;
    hidden: boolean;
    user_comment: string = "";
    camera_shot_key: string = "";
    date_year: string = "";
    date_month: string = "";
    date_day: string = "";
    pending: boolean;
    date_added_ms: long;
    date_modified_ms: long;
    date_trashed_ms: long;
    subtype: PhotoSubtype;
  }

  class MovingPhotoHandle implements MovingPhoto {
    private nativeMovingPhoto: long = 0;

    constructor(context: long) {
      if (this.nativeMovingPhoto == 0) {
        this.nativeMovingPhoto = context;
      }
    }

    private static native transferToDynamicMovingPhoto(input: MovingPhotoHandle): Any;
    private static native transferToStaticMovingPhoto(input: ESValue): MovingPhotoHandle;

    static transferDynamic(input: Object): Any {
      return MovingPhotoHandle.transferToDynamicMovingPhoto(input as MovingPhotoHandle);
    }
    static transferStatic(input: Any): Object {
      if (input === undefined || input === null) {
        throw new Error("Input is null or undefined");
      }
      let dyESValue: ESValue = ESValue.wrap(input);
      if( dyESValue === undefined || dyESValue === null) {
        throw new Error("Input is not a valid ESValue");
      }
      let output: MovingPhotoHandle = MovingPhotoHandle.transferToStaticMovingPhoto(dyESValue);
      if (output === null) {
        throw new Error("Failed to transfer to MovingPhotoHandle");
      }
      return output as Object;
    }


    requestContent(imageFileUri: string, videoFileUri: string): Promise<void> {
      return new Promise<void>((resolve: (v: undefined) => void, reject: (e: BusinessError<void>) => void): void => {
        let cb = (): undefined | MediaLibraryAniError => {
          try {
            this.requestContentByImageFileAndVideoFile(imageFileUri, videoFileUri);
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

    requestContent(resourceType: ResourceType, fileUri: string): Promise<void> {
      return new Promise<void>((resolve: (v: undefined) => void, reject: (e: BusinessError<void>) => void): void => {
        let cb = (): undefined | MediaLibraryAniError => {
          try {
            this.requestContentByResourceTypeAndFile(resourceType, fileUri);
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

    requestContent(resourceType: ResourceType): Promise<ArrayBuffer> {
      return new Promise<ArrayBuffer>((resolve: (v: ArrayBuffer) => void, reject: RejectString): void => {
        let cb = (): ArrayBuffer | MediaLibraryAniError => {
          let result: ArrayBuffer | MediaLibraryAniError;
          try {
            result = this.requestContentByResourceType(resourceType);
          } catch(e) {
            result = e as MediaLibraryAniError;
          }
          return result;
        };
        taskpool.execute(cb).then((ret: Any): void => {
          if (ret instanceof MediaLibraryAniError) {
            let err = ret as MediaLibraryAniError;
            reject(businessError<string>(err.code as int, err.message));
          } else {
            if (isNullish(ret)) {
              reject(businessError<string>(-1, "Operation failed"));
            } else {
              resolve(ret as ArrayBuffer);
            }
          }
        });
      });
    }

    isVideoReady(): Promise<boolean> {
      return new Promise<boolean>((resolve: (v: boolean) => void, reject: RejectString) => {
        let cb = (): boolean | MediaLibraryAniError => {
          let result: boolean | MediaLibraryAniError;
          try {
            result = this.isVideoReadyInner();
          } catch(e) {
            result = e as MediaLibraryAniError;
          }
          return result;
        };
        taskpool.execute(cb).then((ret: Any): void => {
          if (ret instanceof MediaLibraryAniError) {
            let err = ret as MediaLibraryAniError;
            reject(businessError<string>(err.code as int, err.message));
          } else {
            if (isNullish(ret)) {
              reject(businessError<string>(-1, "Operation failed"));
            } else {
              resolve(ret as boolean);
            }
          }
        });
      });
    }

    native getUri(): string;
    native requestContentByImageFileAndVideoFile(imageFileUri: string, videoFileUri: string): void;
    native requestContentByResourceTypeAndFile(resourceType: ResourceType, fileUri: string): void;
    native requestContentByResourceType(resourceType: ResourceType): ArrayBuffer;
    native isVideoReadyInner(): boolean;
  }

  export enum HighlightAlbumInfoType {
    COVER_INFO = 0,
    PLAY_INFO
  }

  export enum HighlightUserActionType {
    INSERTED_PIC_COUNT = 0,
    REMOVED_PIC_COUNT,
    SHARED_SCREENSHOT_COUNT,
    SHARED_COVER_COUNT,
    RENAMED_COUNT,
    CHANGED_COVER_COUNT,
    RENDER_VIEWED_TIMES = 100,
    RENDER_VIEWED_DURATION,
    ART_LAYOUT_VIEWED_TIMES,
    ART_LAYOUT_VIEWED_DURATION
  }

  export enum ThumbnailType {
    LCD = 1,
    THM = 2
  }

  export class HighlightAlbum {
    private nativePhotoAlbum: long = 0;

    constructor(album: Album) {
      let nativePtr: long = this.nativeConstructor(album);
      if (nativePtr != 0) {
        this.nativePhotoAlbum = nativePtr;
      }
    }

    getHighlightAlbumInfo(type: HighlightAlbumInfoType): Promise<string> {
      return new Promise<string>((resolve: (v: string) => void, reject: RejectString) => {
        let cb = (): string | MediaLibraryAniError => {
          let result: string | MediaLibraryAniError;
          try {
            result = this.getHighlightAlbumInfoInner(type);
          } catch(e) {
            result = e as MediaLibraryAniError;
          }
          return result;
        };
        taskpool.execute(cb).then((ret: Any): void => {
          if (ret instanceof MediaLibraryAniError) {
            let err = ret as MediaLibraryAniError;
            reject(businessError<string>(err.code as int, err.message));
          } else {
            if (isNullish(ret)) {
              reject(businessError<string>(-1, "Operation failed"));
            } else {
              resolve(ret as string);
            }
          }
        });
      });
    }

    getHighlightResource(resourceUri: string): Promise<ArrayBuffer> {
      return new Promise<ArrayBuffer>((resolve: (v: ArrayBuffer) => void, reject: RejectString) => {
        let cb = (): ArrayBuffer | MediaLibraryAniError => {
          let result: ArrayBuffer | MediaLibraryAniError;
          try {
            result = this.getHighlightResourceInner(resourceUri);
          } catch(e) {
            result = e as MediaLibraryAniError;
          }
          return result;
        };
        taskpool.execute(cb).then((ret: Any): void => {
          if (ret instanceof MediaLibraryAniError) {
            let err = ret as MediaLibraryAniError;
            reject(businessError<string>(err.code as int, err.message));
          } else {
            if (isNullish(ret)) {
              reject(businessError<string>(-1, "Operation failed"));
            } else {
              resolve (ret as ArrayBuffer);
            }
          }
        });
      });
    }

    setHighlightUserActionData(type: HighlightUserActionType, actionData: int): Promise<void> {
      return new Promise<void>((resolve: (v: undefined) => void, reject: (e: BusinessError<void>) => void): void => {
        let cb = (): MediaLibraryAniError | undefined => {
          try {
            this.setHighlightUserActionDataInner(type, actionData);
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

    setSubTitle(subTitle: string): Promise<void> {
      return new Promise<void>((resolve: (v: undefined) => void, reject: (e: BusinessError<void>) => void): void => {
        let cb = (): MediaLibraryAniError | undefined => {
          try {
            this.setSubTitleInner(subTitle);
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

    static deleteHighlightAlbums(context: Context, albums: Array<Album>): Promise<int> {
      return new Promise<int>((resolve: (v: int) => void, reject: RejectString) : void => {
        let cb = (): int | MediaLibraryAniError => {
          let result: int | MediaLibraryAniError;
          try {
            result = HighlightAlbum.deleteHighlightAlbumsInner(context, albums);
          } catch(e) {
            result = e as MediaLibraryAniError;
          }
          return result;
        };
        taskpool.execute(cb).then((ret: Any): void => {
          if (ret instanceof MediaLibraryAniError) {
            let err = ret as MediaLibraryAniError;
            reject(businessError<string>(err.code as int, err.message));
          } else {
            if (isNullish(ret)) {
              reject(businessError<string>(-1, "Operation failed"));
            } else {
              resolve(ret as int);
            }
          }
        });
      });
    }

    native nativeConstructor(album: Album): long;
    native getHighlightAlbumInfoInner(type: HighlightAlbumInfoType): string;
    native getHighlightResourceInner(resourceUri: string): ArrayBuffer;
    native setHighlightUserActionDataInner(type: HighlightUserActionType, actionData: int): void;
    native setSubTitleInner(subTitle: string): void;
    native static deleteHighlightAlbumsInner(context: Context, albums: Array<Album>): int;
  }

  export class AnalysisAlbum {
    private nativePhotoAlbum: long = 0;

    constructor(album: Album) {
      let nativePtr: long = this.nativeConstructor(album);
      if (nativePtr != 0) {
        this.nativePhotoAlbum = nativePtr;
      }
    }

    getOrderPosition(assets: Array<PhotoAsset>): Promise<Array<int>> {
      return new Promise<Array<int>>((resolve: (v: Array<int>) => void, reject: RejectString) => {
        let cb = (): Array<int> | MediaLibraryAniError => {
          let result: Array<int> | MediaLibraryAniError;
          try {
            result = this.getOrderPositionInner(assets);
          } catch(e) {
            result = e as MediaLibraryAniError;
          }
          return result;
        };
        taskpool.execute(cb).then((ret: Any): void => {
          if (ret instanceof MediaLibraryAniError) {
            let err = ret as MediaLibraryAniError;
            reject(businessError<string>(err.code as int, err.message));
          } else {
            if (isNullish(ret)) {
              reject(businessError<string>(-1, "Operation failed"));
            } else {
              resolve(ret as Array<int>);
            }
          }
        });
      });
    }

    native nativeConstructor(album: Album): long;
    native getOrderPositionInner(assets: Array<PhotoAsset>): Array<int>;
  }

  export enum CloudEnhancementTaskStage {
    TASK_STAGE_EXCEPTION = -1,
    TASK_STAGE_PREPARING,
    TASK_STAGE_UPLOADING,
    TASK_STAGE_EXECUTING,
    TASK_STAGE_DOWNLOADING,
    TASK_STAGE_FAILED,
    TASK_STAGE_COMPLETED
  }

  export interface CloudEnhancementTaskState {
    readonly taskStage: CloudEnhancementTaskStage;
    readonly transferredFileSize?: int;
    readonly totalFileSize?: int;
    readonly expectedDuration?: int;
    readonly statusCode?: int;
  }

  class CloudEnhancementTaskStateHandle implements CloudEnhancementTaskState {
    readonly taskStage: CloudEnhancementTaskStage = CloudEnhancementTaskStage.TASK_STAGE_EXCEPTION;
    readonly transferredFileSize: int | undefined = undefined;
    readonly totalFileSize: int | undefined = undefined;
    readonly expectedDuration: int | undefined = undefined;
    readonly statusCode: int | undefined = undefined;
    private nativeHandle: long = 0;

    private constructor(nativePointer: long) {
      if (this.nativeHandle == 0) {
        this.nativeHandle = nativePointer;
        let undefinedValue: int = -1;
        this.transferredFileSize =
          this.getTransferredFileSize() == undefinedValue ? undefined : this.getTransferredFileSize();
        this.totalFileSize = this.getTotalFileSize() == undefinedValue ? undefined : this.getTotalFileSize();
        this.expectedDuration = this.getExpectedDuration() == undefinedValue ? undefined : this.getExpectedDuration();
        this.statusCode = this.getStatusCode() == undefinedValue ? undefined : this.getStatusCode();
      }
    }

    private native getTransferredFileSize(): int;
    private native getTotalFileSize(): int;
    private native getExpectedDuration(): int;
    private native getStatusCode(): int;
  }

  export class CloudEnhancement {
    private nativeHandle: long = 0;

    constructor(nativePointer: long) {
      if (this.nativeHandle == 0) {
        this.nativeHandle = nativePointer;
      }
    }

    native static getCloudEnhancementInstance(context: Context): CloudEnhancement;

    submitCloudEnhancementTasks(photoAssets: Array<PhotoAsset>, hasCloudWatermark: boolean): Promise<void> {
      return new Promise<void>((resolve: (v: undefined) => void, reject: (e: BusinessError<void>) => void): void => {
        let cb = (): MediaLibraryAniError | undefined => {
          try {
            this.submitCloudEnhancementTasksSync(photoAssets, hasCloudWatermark);
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
            reject(businessError (err.code as int, err.message));
          }
        });
      });
    }

    submitCloudEnhancementTasks(photoAssets: Array<PhotoAsset>, hasCloudWatermark: boolean,
      triggerMode?: int): Promise<void> {
      return new Promise<void>((resolve: (v: undefined) => void, reject: (e: BusinessError<void>) => void): void => {
        let cb = (): MediaLibraryAniError | undefined => {
          try {
            this.submitCloudEnhancementTasksSync(photoAssets, hasCloudWatermark, triggerMode);
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

    prioritizeCloudEnhancementTask(photoAsset: PhotoAsset): Promise<void> {
      return new Promise<void>((resolve: (v: undefined) => void, reject: (e: BusinessError<void>) => void): void => {
        let cb = (): MediaLibraryAniError | undefined => {
          try {
            this.prioritizeCloudEnhancementTaskSync(photoAsset);
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

    cancelCloudEnhancementTasks(photoAssets: Array<PhotoAsset>): Promise<void> {
      return new Promise<void>((resolve: (v: undefined) => void, reject: (e: BusinessError<void>) => void): void => {
        let cb = (): MediaLibraryAniError | undefined => {
          try {
            this.cancelCloudEnhancementTasksSync(photoAssets);
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

    cancelAllCloudEnhancementTasks(): Promise<void> {
      return new Promise<void>((resolve: (v: undefined) => void, reject: (e: BusinessError<void>) => void): void => {
        let cb = (): MediaLibraryAniError | undefined => {
          try {
            this.cancelAllCloudEnhancementTasksSync();
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

    queryCloudEnhancementTaskState(photoAsset: PhotoAsset): Promise<CloudEnhancementTaskState> {
      return new Promise<CloudEnhancementTaskState>((
        resolve: (v: CloudEnhancementTaskState) => void, reject: RejectString) => {
        let cb = (): CloudEnhancementTaskState | MediaLibraryAniError => {
          let result: CloudEnhancementTaskState | MediaLibraryAniError;
          try {
            result = this.queryCloudEnhancementTaskStateSync(photoAsset);
          } catch(e) {
            result = e as MediaLibraryAniError;
          }
          return result;
        };
        taskpool.execute(cb).then((ret: Any): void => {
          if(ret instanceof MediaLibraryAniError) {
            let err = ret as MediaLibraryAniError;
            reject(businessError<string>(err.code as int, err.message));
          } else {
            if (isNullish(ret)) {
              reject(businessError<string>(-1, "Operation failed"));
            } else {
              resolve(ret as CloudEnhancementTaskState);
            }
          }
        });
      });
    }

    syncCloudEnhancementTaskStatus(): Promise<void> {
      return new Promise<void>((resolve: (v: undefined) => void, reject: (e: BusinessError<void>) => void): void => {
        let cb = (): MediaLibraryAniError | undefined => {
          try {
            this.syncCloudEnhancementTaskStatusSync();
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

    getCloudEnhancementPair(asset: PhotoAsset): Promise<PhotoAsset> {
      return new Promise<PhotoAsset>((resolve: (v: PhotoAsset) => void, reject: RejectString) => {
        let cb = (): PhotoAsset | MediaLibraryAniError => {
          let result: PhotoAsset | MediaLibraryAniError;
          try {
            result = this.getCloudEnhancementPairSync(asset);
          } catch(e) {
            result = e as MediaLibraryAniError;
          }
          return result;
        };
        taskpool.execute(cb).then((ret: Any): void => {
          if (ret instanceof MediaLibraryAniError) {
            let err = ret as MediaLibraryAniError;
            reject(businessError<string>(err.code as int, err.message));
          } else {
            if (isNullish(ret)) {
              reject(businessError<string>(-1, "Operation failed"));
            } else {
              resolve(ret as PhotoAsset);
            }
          }
        });
      });
    }

    private native submitCloudEnhancementTasksSync(photoAssets: Array<PhotoAsset>, hasCloudWatermark: boolean,
                                                   triggerMode?: int): void;
    private native prioritizeCloudEnhancementTaskSync(photoAsset: PhotoAsset): void;
    private native cancelCloudEnhancementTasksSync(photoAssets: Array<PhotoAsset>): void;
    private native cancelAllCloudEnhancementTasksSync(): void;
    private native queryCloudEnhancementTaskStateSync(photoAsset: PhotoAsset): CloudEnhancementTaskState;
    private native syncCloudEnhancementTaskStatusSync(): void;
    private native getCloudEnhancementPairSync(asset: PhotoAsset): PhotoAsset;
  }

  export enum CloudEnhancementState {
    UNAVAILABLE = 0,
    AVAILABLE,
    EXECUTING,
    COMPLETED
  }

  export enum CloudMediaAssetTaskStatus  {
    DOWNLOADING = 0,
    PAUSED,
    IDLE,
  };

  export enum CloudMediaTaskPauseCause {
    NO_PAUSE = 0,
    TEMPERATURE_LIMIT,
    ROM_LIMIT,
    NETWORK_FLOW_LIMIT,
    WIFI_UNAVAILABLE,
    POWER_LIMIT,
    BACKGROUND_TASK_UNAVAILABLE,
    FREQUENT_USER_REQUESTS,
    CLOUD_ERROR,
    USER_PAUSED,
  };

  export interface CloudMediaAssetStatus {
    readonly taskStatus: CloudMediaAssetTaskStatus;
    readonly taskInfo: string;
    readonly errorCode: CloudMediaTaskPauseCause;
  };

  class CloudMediaAssetStatusHandle implements CloudMediaAssetStatus {
    readonly taskStatus: CloudMediaAssetTaskStatus = CloudMediaAssetTaskStatus.IDLE;
    readonly taskInfo: string = "";
    readonly errorCode: CloudMediaTaskPauseCause = CloudMediaTaskPauseCause.NO_PAUSE;
    private nativeHandle: long = 0;

    private constructor(nativePointer: long) {
      if (this.nativeHandle == 0) {
        this.nativeHandle = nativePointer;
        this.taskStatus = this.gettaskStatus();
        this.taskInfo = this.gettaskInfo();
        this.errorCode = this.geterrorCode();
      }
    }

    private native gettaskStatus(): CloudMediaAssetTaskStatus;
    private native gettaskInfo(): string;
    private native geterrorCode(): CloudMediaTaskPauseCause;
  }

  export enum CloudMediaDownloadType{
    DOWNLOAD_FORCE = 0,
    DOWNLOAD_GENTLE,
  };

  export enum CloudMediaRetainType{
    RETAIN_FORCE = 0,
  };

  export class CloudMediaAssetManager {
    private nativeHandle: long = 0;

    constructor(nativePointer: long) {
      if (this.nativeHandle == 0) {
        this.nativeHandle = nativePointer;
      }
    }
    native static getCloudMediaAssetManagerInstance(context: Context): CloudMediaAssetManager;

    startDownloadCloudMedia(downloadType: CloudMediaDownloadType): Promise<void> {
      return new Promise<void>((resolve: (v: undefined) => void, reject: (e: BusinessError<void>) => void): void => {
        let cb = (): MediaLibraryAniError | undefined => {
          try {
            this.startDownloadCloudMediaInner(downloadType);
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

    pauseDownloadCloudMedia(): Promise<void> {
      return new Promise<void>((resolve: (v: undefined) => void, reject: (e: BusinessError<void>) => void): void => {
        let cb = (): MediaLibraryAniError | undefined => {
          try {
            this.pauseDownloadCloudMediaInner();
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

    cancelDownloadCloudMedia(): Promise<void> {
      return new Promise<void>((resolve: (v: undefined) => void, reject: (e: BusinessError<void>) => void): void => {
        let cb = (): MediaLibraryAniError | undefined => {
          try {
            this.cancelDownloadCloudMediaInner();
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

    retainCloudMediaAsset(retainType: CloudMediaRetainType): Promise<void> {
      return new Promise<void>((resolve: (v: undefined) => void, reject: (e: BusinessError<void>) => void): void => {
        let cb = (): MediaLibraryAniError | undefined => {
          try {
            this.retainCloudMediaAssetInner(retainType);
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

    getCloudMediaAssetStatus(): Promise<CloudMediaAssetStatus> {
      return new Promise<CloudMediaAssetStatus>(
        (resolve: (v: CloudMediaAssetStatus) => void, reject: RejectString) => {
        let cb = (): CloudMediaAssetStatus | MediaLibraryAniError => {
          let result: CloudMediaAssetStatus | MediaLibraryAniError;
          try {
            result = this.getCloudMediaAssetStatusInner();
          } catch(e) {
            result = e as MediaLibraryAniError;
          }
          return result;
        };
        taskpool.execute(cb).then((ret: Any): void => {
          if (ret instanceof MediaLibraryAniError) {
            let err = ret as MediaLibraryAniError;
            reject(businessError<string>(err.code as int, err.message));
          } else {
            if (isNullish(ret)) {
              reject(businessError<string>(-1, "Operation failed"));
            } else {
              resolve(ret as CloudMediaAssetStatus);
            }
          }
        });
      });
    }

    private native startDownloadCloudMediaInner(downloadType: CloudMediaDownloadType): void;
    private native pauseDownloadCloudMediaInner(): void;
    private native cancelDownloadCloudMediaInner(): void;
    private native retainCloudMediaAssetInner(retainType: CloudMediaRetainType): void;
    private native getCloudMediaAssetStatusInner(): CloudMediaAssetStatus;
  }

  export interface PhotoAssetCustomRecord {
    readonly fileId: int;
    readonly shareCount: int;
    readonly lcdJumpCount: int;
  }

  export class PhotoAssetCustomRecordManager {
    private nativeHandle: long = 0;

    constructor(nativePointer: long) {
      if (this.nativeHandle == 0) {
        this.nativeHandle = nativePointer;
      }
    }
    native static getCustomRecordManagerInstance(context: Context): PhotoAssetCustomRecordManager | null;

    createCustomRecords(customRecords: Array<PhotoAssetCustomRecord>): Promise<void> {
      return new Promise<void>((resolve: (v: undefined) => void, reject: (e: BusinessError<void>) => void): void => {
        let cb = (): MediaLibraryAniError | undefined => {
          try {
            this.createCustomRecordsInner(customRecords);
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

    addShareCount(ids: Array<int>): Promise<Array<int>> {
      return new Promise<Array<int>>((resolve: (v: Array<int>) => void, reject: RejectString) => {
        let cb = (): Array<int> | MediaLibraryAniError => {
          let result: Array<int> | MediaLibraryAniError;
          try {
            result = this.addShareCountInner(ids);
          } catch(e) {
            result = e as MediaLibraryAniError;
          }
          return result;
        };
        taskpool.execute(cb).then((ret: Any): void => {
          if (ret instanceof MediaLibraryAniError) {
            let err = ret as MediaLibraryAniError;
            reject(businessError<string>(err.code as int, err.message));
          } else {
            if (isNullish(ret)) {
              reject(businessError<string>(-1, "Operation failed"));
            } else {
              resolve(ret as Array<int>);
            }
          }
        });
      });
    }

    addLcdJumpCount(ids: Array<int>): Promise<Array<int>> {
      return new Promise<Array<int>>((resolve: (v: Array<int>) => void, reject: RejectString) => {
        let cb = (): Array<int> | MediaLibraryAniError => {
          let result: Array<int> | MediaLibraryAniError;
          try {
            result = this.addLcdJumpCountInner(ids);
          } catch(e) {
            result = e as MediaLibraryAniError;
          }
          return result;
        };
        taskpool.execute(cb).then((ret: Any): void => {
          if (ret instanceof MediaLibraryAniError) {
            let err = ret as MediaLibraryAniError;
            reject(businessError<string>(err.code as int, err.message));
          } else {
            if (isNullish(ret)) {
              reject(businessError<string>(-1, "Operation failed"));
            } else {
              resolve(ret as Array<int>);
            }
          }
        });
      });
    }

    removeCustomRecords(optionCheck: FetchOptions): Promise<void> {
      return new Promise<void>((resolve: (v: undefined) => void, reject: (e: BusinessError<void>) => void): void => {
        let cb = (): MediaLibraryAniError | undefined => {
          try {
            this.removeCustomRecordsInner(optionCheck);
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

    setCustomRecords(customRecords: Array<PhotoAssetCustomRecord>): Promise<Array<int>> {
      return new Promise<Array<int>>((resolve: (v: Array<int>) => void, reject: RejectString) => {
        let cb = (): Array<int> | MediaLibraryAniError => {
          let result: Array<int> | MediaLibraryAniError;
          try {
            result = this.setCustomRecordsInner(customRecords);
          } catch(e) {
            result = e as MediaLibraryAniError;
          }
          return result;
        };
        taskpool.execute(cb).then((ret: Any): void => {
          if (ret instanceof MediaLibraryAniError) {
            let err = ret as MediaLibraryAniError;
            reject(businessError<string>(err.code as int, err.message));
          } else {
            if (isNullish(ret)) {
              reject(businessError<string>(-1, "Operation failed"));
            } else {
              resolve(ret as Array<int>);
            }
          }
        });
      });
    }

    getCustomRecords(optionCheck: FetchOptions): Promise<FetchResult<PhotoAssetCustomRecord>> {
      return new Promise<FetchResult<PhotoAssetCustomRecord>>((resolve: (v: FetchResult<PhotoAssetCustomRecord>) => void,
        reject: RejectString) => {
        let cb = (): FetchResult<PhotoAssetCustomRecord> | MediaLibraryAniError => {
          let result: MediaLibraryAniError | FetchResult<PhotoAssetCustomRecord>;
          try {
            result =  this.getCustomRecordsInner(optionCheck);
          } catch(e) {
            result =  e as MediaLibraryAniError;
          }
          return result;
        };
        taskpool.execute(cb).then((ret: Any): void => {
          if (ret instanceof MediaLibraryAniError) {
            let err = ret as MediaLibraryAniError;
            reject(businessError<string>(err.code as int, err.message));
          } else {
            if (isNullish(ret)) {
              reject(businessError<string>(-1, "Operation failed"));
            } else {
              resolve(ret as FetchResult<PhotoAssetCustomRecord>);
            }
          }
        });
      });
    }

    private native createCustomRecordsInner(customRecords: Array<PhotoAssetCustomRecord>): void;
    private native addShareCountInner(ids: Array<int>): Array<int>;
    private native addLcdJumpCountInner(ids: Array<int>): Array<int>;
    private native removeCustomRecordsInner(optionCheck: FetchOptions): void;
    private native setCustomRecordsInner(customRecords: Array<PhotoAssetCustomRecord>): Array<int>;
    private native getCustomRecordsInner(optionCheck: FetchOptions): FetchResult<PhotoAssetCustomRecord>;
  }
}
