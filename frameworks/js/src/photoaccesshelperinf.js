/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

const photoAccessHelper = requireInternal('file.photoAccessHelper');
const bundleManager = requireNapi('bundle.bundleManager');

const ARGS_TWO = 2;
const ARGS_THREE = 3;

const WRITE_PERMISSION = 'ohos.permission.WRITE_IMAGEVIDEO';

const PERMISSION_DENIED = 13900012;
const ERR_CODE_PARAMERTER_INVALID = 13900020;
const ERR_CODE_OHOS_PERMISSION_DENIED = 201;
const ERR_CODE_OHOS_PARAMERTER_INVALID = 401;
const REQUEST_CODE_SUCCESS = 0;
const PERMISSION_STATE_ERROR = -1;
const ERROR_MSG_WRITE_PERMISSION = 'not have ohos.permission.WRITE_IMAGEVIDEO';
const ERROR_MSG_USER_DENY = 'user deny';
const ERROR_MSG_PARAMERTER_INVALID = 'input parmaeter invalid';
const ERROR_MSG_INNER_FAIL = 'System inner fail';

const MAX_DELETE_NUMBER = 300;
const MIN_DELETE_NUMBER = 1;

let gContext = undefined;

class BusinessError extends Error {
  constructor(msg, code) {
    super(msg);
    this.code = code || PERMISSION_DENIED;
  }
}
function checkParams(uriList, asyncCallback) {
  if (arguments.length > ARGS_TWO) {
    return false;
  }
  if (!Array.isArray(uriList)) {
    return false;
  }
  if (asyncCallback && typeof asyncCallback !== 'function') {
    return false;
  }
  if (uriList.length < MIN_DELETE_NUMBER || uriList.length > MAX_DELETE_NUMBER) {
    return false;
  }
  let tag = 'file://media/Photo/';
  for (let uri of uriList) {
    if (!uri.includes(tag)) {
      console.info(`photoAccessHelper invalid uri: ${uri}`);
      return false;
    }
  }
  return true;
}
function errorResult(rej, asyncCallback) {
  if (asyncCallback) {
    return asyncCallback(rej);
  }
  return new Promise((resolve, reject) => {
    reject(rej);
  });
}

function getAbilityResource(bundleInfo) {
  let labelId = bundleInfo.abilitiesInfo[0].labelId;
  for (let abilityInfo of bundleInfo.abilitiesInfo) {
    if (abilityInfo.name === bundleInfo.mainElementName) {
      labelId = abilityInfo.labelId;
    }
  }

  return labelId;
}

async function getAppName() {
  let appName = '';
  try {
    const flags = bundleManager.BundleFlag.GET_BUNDLE_INFO_WITH_ABILITY | bundleManager.BundleFlag.GET_BUNDLE_INFO_WITH_HAP_MODULE;
    const bundleInfo = await bundleManager.getBundleInfoForSelf(flags);
    console.info(`photoAccessHelper bundleInfo: ${JSON.stringify(bundleInfo)}`)
    if (bundleInfo === undefined || bundleInfo.hapModulesInfo === undefined || bundleInfo.hapModulesInfo.length === 0) {
      return appName;
    }
    const labelId = getAbilityResource(bundleInfo.hapModulesInfo[0]);
    const resourceMgr = gContext.resourceManager;
    appName = await resourceMgr.getStringValue(labelId);
    console.info(`photoAccessHelper appName: ${appName}`)
  } catch (error) {
    console.info(`photoAccessHelper error: ${JSON.stringify(error)}`)
  }

  return appName;
}

async function createPhotoDeleteRequestParamsOk(uriList, asyncCallback) {
  let flags = bundleManager.BundleFlag.GET_BUNDLE_INFO_WITH_REQUESTED_PERMISSION;
  let { reqPermissionDetails, permissionGrantStates } = await bundleManager.getBundleInfoForSelf(flags);
  let permissionIndex = -1;
  for (let i = 0; i < reqPermissionDetails.length; i++) {
    if (reqPermissionDetails[i].name === WRITE_PERMISSION) {
      permissionIndex = i;
    }
  }
  if (permissionIndex < 0 || permissionGrantStates[permissionIndex] === PERMISSION_STATE_ERROR) {
    console.info('photoAccessHelper permission error');
    return errorResult(new BusinessError(ERROR_MSG_WRITE_PERMISSION), asyncCallback);
  }
  const appName = await getAppName();
  if (appName.length === 0) {
    console.info(`photoAccessHelper appName not found`);
    return errorResult(new BusinessError(ERROR_MSG_PARAMERTER_INVALID, ERR_CODE_PARAMERTER_INVALID), asyncCallback);
  }
  try {
    if (asyncCallback) {
      return photoAccessHelper.createDeleteRequest(getContext(this), appName, uriList, result => {
        if (result.result === REQUEST_CODE_SUCCESS) {
          asyncCallback();
        } else if (result.result == PERMISSION_DENIED) {
          asyncCallback(new BusinessError(ERROR_MSG_USER_DENY));
        } else {
          asyncCallback(new BusinessError(ERROR_MSG_INNER_FAIL, result.result));
        }
      });
    } else {
      return new Promise((resolve, reject) => {
        photoAccessHelper.createDeleteRequest(getContext(this), appName, uriList, result => {
          if (result.result === REQUEST_CODE_SUCCESS) {
            resolve();
          } else if (result.result == PERMISSION_DENIED) {
            reject(new BusinessError(ERROR_MSG_USER_DENY));
          } else {
            reject(new BusinessError(ERROR_MSG_INNER_FAIL, result.result));
          }
        });
      });
    }
  } catch (error) {
    return errorResult(new BusinessError(error.message, error.code), asyncCallback);
  }
}

function createDeleteRequest(...params) {
  if (!checkParams(...params)) {
    throw new BusinessError(ERROR_MSG_PARAMERTER_INVALID, ERR_CODE_PARAMERTER_INVALID);
  }
  return createPhotoDeleteRequestParamsOk(...params);
}

function getPhotoAccessHelper(context) {
  if (context === undefined) {
    console.log('photoAccessHelper gContext undefined');
    throw Error('photoAccessHelper gContext undefined');
  }
  gContext = context;
  let helper = photoAccessHelper.getPhotoAccessHelper(gContext);
  if (helper !== undefined) {
    console.log('photoAccessHelper getPhotoAccessHelper inner add createDeleteRequest');
    helper.createDeleteRequest = createDeleteRequest;
  }
  return helper;
}

function startPhotoPicker(context, config) {
  if (context === undefined) {
    console.log('photoAccessHelper gContext undefined');
    throw Error('photoAccessHelper gContext undefined');
  }
  if (config === undefined) {
    console.log('photoAccessHelper config undefined');
    throw Error('photoAccessHelper config undefined');
  }
  gContext = context;
  let helper = photoAccessHelper.startPhotoPicker(gContext, config);
  if (helper !== undefined) {
    console.log('photoAccessHelper startPhotoPicker inner add createDeleteRequest');
    helper.createDeleteRequest = createDeleteRequest;
  }
  return helper;
}

function getPhotoAccessHelperAsync(context, asyncCallback) {
  if (context === undefined) {
    console.log('photoAccessHelper gContext undefined');
    throw Error('photoAccessHelper gContext undefined');
  }
  gContext = context;
  if (arguments.length === 1) {
    return photoAccessHelper.getPhotoAccessHelperAsync(gContext)
      .then((helper) => {
        if (helper !== undefined) {
          console.log('photoAccessHelper getPhotoAccessHelperAsync inner add createDeleteRequest');
          helper.createDeleteRequest = createDeleteRequest;
        }
        return helper;
      })
      .catch((err) => {
        console.log('photoAccessHelper getPhotoAccessHelperAsync err ' + err);
        throw Error(err);
      });
  } else if (arguments.length === ARGS_TWO && typeof asyncCallback === 'function') {
    photoAccessHelper.getPhotoAccessHelperAsync(gContext, (err, helper) => {
      console.log('photoAccessHelper getPhotoAccessHelperAsync callback ' + err);
      if (err) {
        asyncCallback(err);
      } else {
        if (helper !== undefined) {
          console.log('photoAccessHelper getPhotoAccessHelperAsync callback add createDeleteRequest');
          helper.createDeleteRequest = createDeleteRequest;
        }
        asyncCallback(err, helper);
      }
    });
  } else {
    console.log('photoAccessHelper getPhotoAccessHelperAsync param invalid');
    throw new BusinessError(ERROR_MSG_PARAMERTER_INVALID, ERR_CODE_PARAMERTER_INVALID);
  }
  return undefined;
}

const RecommendationType = {
  // Indicates that QR code or barcode photos can be recommended
  QR_OR_BAR_CODE: 1,

  // Indicates that QR code photos can be recommended
  QR_CODE: 2,

  // Indicates that barcode photos can be recommended
  BAR_CODE: 3,

  // Indicates that QR code or barcode photos can be recommended
  ID_CARD: 4,

  // Indicates that profile picture photos can be recommended
  PROFILE_PICTURE: 5,

  // Indicates that passport photos can be recommended
  PASSPORT: 6,

  // Indicates that bank card photos can be recommended
  BANK_CARD: 7,

  // Indicates that driver license photos can be recommended
  DRIVER_LICENSE: 8,

  // Indicates that driving license photos can be recommended
  DRIVING_LICENSE: 9
}

const PhotoViewMIMETypes = {
  IMAGE_TYPE: 'image/*',
  VIDEO_TYPE: 'video/*',
  IMAGE_VIDEO_TYPE: '*/*',
  MOVING_PHOTO_IMAGE_TYPE: 'image/movingPhoto',
  INVALID_TYPE: ''
}

const ErrCode = {
  INVALID_ARGS: 13900020,
  RESULT_ERROR: 13900042,
  CONTEXT_NO_EXIST: 16000011,
}

const ERRCODE_MAP = new Map([
  [ErrCode.INVALID_ARGS, 'Invalid argument'],
  [ErrCode.RESULT_ERROR, 'Unknown error'],
  [ErrCode.CONTEXT_NO_EXIST, 'Current ability failed to obtain context'],
]);

const PHOTO_VIEW_MIME_TYPE_MAP = new Map([
  [PhotoViewMIMETypes.IMAGE_TYPE, 'FILTER_MEDIA_TYPE_IMAGE'],
  [PhotoViewMIMETypes.VIDEO_TYPE, 'FILTER_MEDIA_TYPE_VIDEO'],
  [PhotoViewMIMETypes.IMAGE_VIDEO_TYPE, 'FILTER_MEDIA_TYPE_ALL'],
  [PhotoViewMIMETypes.MOVING_PHOTO_IMAGE_TYPE, 'FILTER_MEDIA_TYPE_IMAGE_MOVING_PHOTO'],
]);

const ARGS_ZERO = 0;
const ARGS_ONE = 1;

function checkArguments(args) {
  let checkArgumentsResult = undefined;

  if (args.length === ARGS_TWO && typeof args[ARGS_ONE] !== 'function') {
    checkArgumentsResult = getErr(ErrCode.INVALID_ARGS);
  }

  if (args.length > 0 && typeof args[ARGS_ZERO] === 'object') {
    let option = args[ARGS_ZERO];
    if (option.maxSelectNumber !== undefined) {
      if (option.maxSelectNumber.toString().indexOf('.') !== -1) {
        checkArgumentsResult = getErr(ErrCode.INVALID_ARGS);
      }
    }
  }

  return checkArgumentsResult;
}

function getErr(errCode) {
  return { code: errCode, message: ERRCODE_MAP.get(errCode) };
}

function parsePhotoPickerSelectOption(args) {
  let config = {
    action: 'ohos.want.action.photoPicker',
    type: 'multipleselect',
    parameters: {
      uri: 'multipleselect',
    },
  };

  if (args.length > ARGS_ZERO && typeof args[ARGS_ZERO] === 'object') {
    let option = args[ARGS_ZERO];
    if (option.maxSelectNumber && option.maxSelectNumber > 0) {
      let select = (option.maxSelectNumber === 1) ? 'singleselect' : 'multipleselect';
      config.type = select;
      config.parameters.uri = select;
      config.parameters.maxSelectCount = option.maxSelectNumber;
    }
    if (option.MIMEType && PHOTO_VIEW_MIME_TYPE_MAP.has(option.MIMEType)) {
      config.parameters.filterMediaType = PHOTO_VIEW_MIME_TYPE_MAP.get(option.MIMEType);
    }
    config.parameters.isSearchSupported = option.isSearchSupported === undefined || option.isSearchSupported;
    config.parameters.isPhotoTakingSupported = option.isPhotoTakingSupported === undefined || option.isPhotoTakingSupported;
    config.parameters.isEditSupported = option.isEditSupported === undefined || option.isEditSupported;
    config.parameters.recommendationOptions = option.recommendationOptions;
    config.parameters.preselectedUris = option.preselectedUris;
  }

  return config;
}

function getPhotoPickerSelectResult(args) {
  let selectResult = {
    error: undefined,
    data: undefined,
  };

  if (args.resultCode === 0) {
    let uris = args.uris;
    let isOrigin = args.isOrigin;
    selectResult.data = new PhotoSelectResult(uris, isOrigin);
  } else if (args.resultCode === -1) {
    selectResult.data = new PhotoSelectResult([], undefined);
  } else {
    selectResult.error = getErr(ErrCode.RESULT_ERROR);
  }

  return selectResult;
}

async function photoPickerSelect(...args) {
  let checkArgsResult = checkArguments(args);
  if (checkArgsResult !== undefined) {
    console.log('[picker] Invalid argument');
    throw checkArgsResult;
  }

  const config = parsePhotoPickerSelectOption(args);
  console.log('[picker] config: ' + JSON.stringify(config));

  let context = undefined;
  try {
    context = getContext(this);
  } catch (getContextError) {
    console.error('[picker] getContext error: ' + getContextError);
    throw getErr(ErrCode.CONTEXT_NO_EXIST);
  }
  try {
    if (context === undefined) {
      throw getErr(ErrCode.CONTEXT_NO_EXIST);
    }
    let result = await startPhotoPicker(context, config);
    console.log('[picker] result: ' + JSON.stringify(result));
    const selectResult = getPhotoPickerSelectResult(result);
    console.log('[picker] selectResult: ' + JSON.stringify(selectResult));
    if (args.length === ARGS_TWO && typeof args[ARGS_ONE] === 'function') {
      return args[ARGS_ONE](selectResult.error, selectResult.data);
    } else if (args.length === ARGS_ONE && typeof args[ARGS_ZERO] === 'function') {
      return args[ARGS_ZERO](selectResult.error, selectResult.data);
    }
    return new Promise((resolve, reject) => {
      if (selectResult.data !== undefined) {
        resolve(selectResult.data);
      } else {
        reject(selectResult.error);
      }
    })
  } catch (error) {
    console.error('[picker] error: ' + JSON.stringify(error));
  }
  return undefined;
}

function BaseSelectOptions() {
  this.MIMEType = PhotoViewMIMETypes.INVALID_TYPE;
  this.maxSelectNumber = -1;
  this.isSearchSupported = true;
  this.isPhotoTakingSupported = true;
}

function PhotoSelectOptions() {
  this.MIMEType = PhotoViewMIMETypes.INVALID_TYPE;
  this.maxSelectNumber = -1;
  this.isSearchSupported = true;
  this.isPhotoTakingSupported = true;
  this.isEditSupported = true;
}

function PhotoSelectResult(uris, isOriginalPhoto) {
  this.photoUris = uris;
  this.isOriginalPhoto = isOriginalPhoto;
}

function PhotoViewPicker() {
  this.select = photoPickerSelect;
}

function RecommendationOptions() {
}

class MediaAssetChangeRequest extends photoAccessHelper.MediaAssetChangeRequest {
  static deleteAssets(context, assets, asyncCallback) {
    if (arguments.length > ARGS_THREE || arguments.length < ARGS_TWO) {
      throw new BusinessError(ERROR_MSG_PARAMERTER_INVALID, ERR_CODE_OHOS_PARAMERTER_INVALID);
    }

    try {
      if (asyncCallback) {
        return super.deleteAssets(context, result => {
          if (result.result === REQUEST_CODE_SUCCESS) {
            asyncCallback();
          } else if (result.result === PERMISSION_DENIED) {
            asyncCallback(new BusinessError(ERROR_MSG_USER_DENY, ERR_CODE_OHOS_PERMISSION_DENIED));
          } else {
            asyncCallback(new BusinessError(ERROR_MSG_INNER_FAIL, result.result));
          }
        }, assets, asyncCallback);
      }

      return new Promise((resolve, reject) => {
        super.deleteAssets(context, result => {
          if (result.result === REQUEST_CODE_SUCCESS) {
            resolve();
          } else if (result.result === PERMISSION_DENIED) {
            reject(new BusinessError(ERROR_MSG_USER_DENY, ERR_CODE_OHOS_PERMISSION_DENIED));
          } else {
            reject(new BusinessError(ERROR_MSG_INNER_FAIL, result.result));
          }
        }, assets, (err) => {
          if (err) {
            reject(err);
          } else {
            resolve();
          }
        });
      });
    } catch (error) {
      return errorResult(new BusinessError(error.message, error.code), asyncCallback);
    }
  }
}

export default {
  getPhotoAccessHelper,
  startPhotoPicker,
  getPhotoAccessHelperAsync,
  PhotoType: photoAccessHelper.PhotoType,
  PhotoKeys: photoAccessHelper.PhotoKeys,
  AlbumKeys: photoAccessHelper.AlbumKeys,
  AlbumType: photoAccessHelper.AlbumType,
  AlbumSubtype: photoAccessHelper.AlbumSubtype,
  HighlightAlbum: photoAccessHelper.HighlightAlbum,
  PositionType: photoAccessHelper.PositionType,
  PhotoSubtype: photoAccessHelper.PhotoSubtype,
  NotifyType: photoAccessHelper.NotifyType,
  DefaultChangeUri: photoAccessHelper.DefaultChangeUri,
  HiddenPhotosDisplayMode: photoAccessHelper.HiddenPhotosDisplayMode,
  AnalysisType: photoAccessHelper.AnalysisType,
  HighlightAlbumInfoType: photoAccessHelper.HighlightAlbumInfoType,
  HighlightUserActionType: photoAccessHelper.HighlightUserActionType,
  RequestPhotoType: photoAccessHelper.RequestPhotoType,
  PhotoViewMIMETypes: PhotoViewMIMETypes,
  DeliveryMode: photoAccessHelper.DeliveryMode,
  SourceMode: photoAccessHelper.SourceMode,
  BaseSelectOptions: BaseSelectOptions,
  PhotoSelectOptions: PhotoSelectOptions,
  PhotoSelectResult: PhotoSelectResult,
  PhotoViewPicker: PhotoViewPicker,
  RecommendationType: RecommendationType,
  RecommendationOptions: RecommendationOptions,
  ResourceType: photoAccessHelper.ResourceType,
  MediaAssetEditData: photoAccessHelper.MediaAssetEditData,
  MediaAssetChangeRequest: MediaAssetChangeRequest,
  MediaAssetsChangeRequest: photoAccessHelper.MediaAssetsChangeRequest,
  MediaAlbumChangeRequest: photoAccessHelper.MediaAlbumChangeRequest,
  MediaAssetManager: photoAccessHelper.MediaAssetManager,
  MovingPhoto: photoAccessHelper.MovingPhoto,
};
