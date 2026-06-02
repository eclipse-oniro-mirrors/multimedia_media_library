/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

import BackupExtensionAbility, {BundleVersion} from '@ohos.application.BackupExtensionAbility';
import fs from '@ohos.file.fs';
// @ts-ignore
import mediabackup from '@ohos.multimedia.mediabackup';

const TAG = 'MediaBackupExtAbility';

const galleryAppName = 'com.huawei.photos';
const mediaAppName = 'com.android.providers.media.module';

const UPGRADE_RESTORE : number = 0;
const DUAL_FRAME_CLONE_RESTORE : number = 1;
const CLONE_RESTORE : number = 2;
const I_PHONE_CLONE_RESTORE : number = 3;
const OTHERS_PHONE_CLONE_RESTORE : number = 4;
const LITE_PHONE_CLONE_RESTORE : number = 5;
const CLOUD_BACKUP_RESTORE : number = 6;

const UPGRADE_NAME = '0.0.0.0';
const CLOUD_BACKUP_NAME = '99.99.99.995';
const I_PHONE_FRAME_CLONE_NAME = '99.99.99.997';
const OTHERS_PHONE_FRAME_CLONE_NAME = '99.99.99.998';
const DUAL_FRAME_CLONE_NAME = '99.99.99.999';
const STAT_KEY_RESULT_INFO = 'resultInfo';
const STAT_KEY_TYPE = 'type';
const STAT_KEY_PATH = 'path';
const STAT_KEY_ERROR_CODE = 'errorCode';
const STAT_KEY_ERROR_INFO = 'errorInfo';
const STAT_KEY_INFOS = 'infos';
const STAT_KEY_BACKUP_INFO = 'backupInfo';
const STAT_KEY_SUCCESS_COUNT = 'successCount';
const STAT_KEY_DUPLICATE_COUNT = 'duplicateCount';
const STAT_KEY_FAILED_COUNT = 'failedCount';
const STAT_KEY_DETAILS = 'details';
const STAT_KEY_NUMBER = 'number';
const STAT_KEY_PROGRESS_INFO = 'progressInfo';
const STAT_KEY_NAME = 'name';
const STAT_KEY_PROCESSED = 'processed';
const STAT_KEY_TOTAL = 'total';
const STAT_KEY_IS_PERCENTAGE = 'isPercentage';
const STAT_VALUE_ERROR_INFO = 'ErrorInfo';
const STAT_VALUE_COUNT_INFO = 'CountInfo';
const STAT_TYPE_PHOTO = 'photo';
const STAT_TYPE_VIDEO = 'video';
const STAT_TYPE_AUDIO = 'audio';
const STAT_TYPE_LAKE_PHOTO = 'ancoPhoto';
const STAT_TYPE_LAKE_VIDEO = 'ancoVideo';
const STAT_TYPE_FILE_MANAGER_PHOTO = 'fileManagerPhoto';
const STAT_TYPE_FILE_MANAGER_VIDEO = 'fileManagerVideo';
const STAT_TYPE_PHOTO_VIDEO = 'photo&video';
const STAT_TYPE_UPDATE = 'update';
const STAT_TYPE_THUMBNAIL = 'thumbnail';
const STAT_TYPE_OTHER = 'other';
const STAT_TYPE_ONGOING = 'ongoing';
const STAT_TYPES = [STAT_TYPE_PHOTO, STAT_TYPE_VIDEO, STAT_TYPE_AUDIO,
    STAT_TYPE_LAKE_PHOTO, STAT_TYPE_LAKE_VIDEO, STAT_TYPE_FILE_MANAGER_PHOTO, STAT_TYPE_FILE_MANAGER_VIDEO];
const RESULT_INFO_NUM = 2;
const JS_TYPE_STRING = 'string';
const JS_TYPE_BOOLEAN = 'boolean';
const GALLERY_DB_PATH = '/storage/media/local/files/.backup/restore/gallery.db';
const DEFAULT_RESTORE_EX_INFO = {
  'resultInfo':
  [
    {
      'type': STAT_VALUE_ERROR_INFO,
      'errorCode': '13500099',
      'errorInfo': 'Get restoreEx info failed'
    },
    {
      'type': STAT_VALUE_COUNT_INFO,
      'infos':
      [
        {
          'backupInfo': STAT_TYPE_PHOTO,
          'successCount': 0,
          'duplicateCount': 0,
          'failedCount': 0,
          'details': null
        },
        {
          'backupInfo': STAT_TYPE_VIDEO,
          'successCount': 0,
          'duplicateCount': 0,
          'failedCount': 0,
          'details': null
        },
        {
          'backupInfo': STAT_TYPE_AUDIO,
          'successCount': 0,
          'duplicateCount': 0,
          'failedCount': 0,
          'details': null
        },
        {
          'backupInfo': STAT_TYPE_LAKE_PHOTO,
          'successCount': 0,
          'duplicateCount': 0,
          'failedCount': 0,
          'details': null
        },
        {
          'backupInfo': STAT_TYPE_LAKE_VIDEO,
          'successCount': 0,
          'duplicateCount': 0,
          'failedCount': 0,
          'details': null
        },
        {
          'backupInfo': STAT_TYPE_FILE_MANAGER_PHOTO,
          'successCount': 0,
          'duplicateCount': 0,
          'failedCount': 0,
          'details': null
        },
        {
          'backupInfo': STAT_TYPE_FILE_MANAGER_VIDEO,
          'successCount': 0,
          'duplicateCount': 0,
          'failedCount': 0,
          'details': null
        }
      ]
    }
  ]
};
const DEFAULT_BACKUP_INFO = [
  {
    'backupInfo': STAT_TYPE_PHOTO,
    'number': 0
  },
  {
    'backupInfo': STAT_TYPE_VIDEO,
    'number': 0
  },
  {
    'backupInfo': STAT_TYPE_AUDIO,
    'number': 0
  }
];
const DEFAULT_PROGRESS_INFO = {
  'progressInfo': [
  {
    'name': STAT_TYPE_PHOTO_VIDEO,
    'processed': 0,
    'total': 0,
    'isPercentage': false
  },
  {
    'name': STAT_TYPE_AUDIO,
    'processed': 0,
    'total': 0,
    'isPercentage': false
  },
  {
    'name': STAT_TYPE_UPDATE,
    'processed': 0,
    'total': 0,
    'isPercentage': false
  },
  {
    'name': STAT_TYPE_THUMBNAIL,
    'processed': 0,
    'total': 0,
    'isPercentage': false
  },
  {
    'name': STAT_TYPE_OTHER,
    'processed': 0,
    'total': 0,
    'isPercentage': false
  },
  {
    'name': STAT_TYPE_ONGOING,
    'processed': 0,
    'total': 0,
    'isPercentage': false
  }]
};

interface BackupCompatibilityInfo {
  backupHdcEnable: boolean;
}

export default class MediaBackupExtAbility extends BackupExtensionAbility {
  async onBackup() : Promise<void> {
    console.log(TAG, ' onBackup ok.');
    console.time(TAG + ' BACKUP');
    await mediabackup.startBackup(CLONE_RESTORE, galleryAppName, mediaAppName);
    console.timeEnd(TAG + ' BACKUP');
  }

  async onBackupEx(backupInfo: string) : Promise<string> {
    console.log(TAG, 'enter onBackupEx, backupInfo: ' + backupInfo);
    console.time(TAG + ' BACKUPEX');
    let startBackupExResult: string = await mediabackup.startBackupEx(CLONE_RESTORE, galleryAppName, mediaAppName, backupInfo);
    console.log(TAG, ' onBackupEx ret: ' + startBackupExResult);
    console.timeEnd(TAG + ' BACKUPEX');
    return startBackupExResult;
  }

  async getRestoreCompatibilityInfo(extInfo: string): Promise<string> {
    try {
      console.log(TAG, ' enter getRestoreCompatibilityInfo, extInfo: ' + extInfo);
      console.time(TAG + ' getRestoreCompatibilityInfo');
      let backupCompatibilityInfo: BackupCompatibilityInfo = {
        backupHdcEnable: true,
      };
      let ret = JSON.stringify(backupCompatibilityInfo);
      console.log(TAG, ' getRestoreCompatibilityInfo ret: ' + ret);
      console.timeEnd(TAG + ' getRestoreCompatibilityInfo');
      return ret;
    } catch (error) {
      console.error(`getRestoreCompatibilityInfo failed with error. Code: ${error.code}, message: ${error.message}`);
    }
    return '';
  }

  async onRelease(scenario: number): Promise<void> {
    try {
      console.log(TAG, ' enter onRelease.');
      console.time(TAG + ' RELEASE');
      await mediabackup.release(this.context, CLONE_RESTORE, scenario);
      console.timeEnd(TAG + ' RELEASE');
    } catch (error) {
      console.error(`onRelease failed with error. Code: ${error.code}, Message: ${error.message}`);
    }
  }

  async onRestore(bundleVersion : BundleVersion) : Promise<void> {
    console.log(TAG, `onRestore ok ${JSON.stringify(bundleVersion)}`);
    console.time(TAG + ' RESTORE');
    const backupDir = this.context.backupDir + 'restore';
    let sceneCode: number = this.getSceneCode(bundleVersion);
    await mediabackup.startRestore(this.context, sceneCode, galleryAppName, mediaAppName, backupDir);
    console.timeEnd(TAG + ' RESTORE');
  }

  async onRestoreEx(bundleVersion: BundleVersion, bundleInfo: string): Promise<string> {
    console.log(TAG, `onRestoreEx ok ${JSON.stringify(bundleVersion)}`);
    console.time(TAG + ' RESTORE EX');
    const backupDir = this.context.backupDir + 'restore';
    let sceneCode: number = this.getSceneCode(bundleVersion);
    let restoreExResult: string = await mediabackup.startRestoreEx(this.context, sceneCode, galleryAppName, mediaAppName, backupDir,
      bundleInfo);
    let restoreExInfo: string = await this.getRestoreExInfo(sceneCode, restoreExResult);
    console.log(TAG, `GET restoreExInfo: ${restoreExInfo}`);
    console.timeEnd(TAG + ' RESTORE EX');
    return restoreExInfo;
  }

  getBackupInfo(): string {
    console.log(TAG, 'getBackupInfo ok');
    let tmpBackupInfo: string = mediabackup.getBackupInfo(CLONE_RESTORE);
    let backupInfo: string;
    if (!this.isBackupInfoValid(tmpBackupInfo)) {
      console.error(TAG, 'backupInfo is invalid, return default');
      backupInfo = JSON.stringify(DEFAULT_BACKUP_INFO);
    } else {
      backupInfo = tmpBackupInfo;
    }
    console.log(TAG, `GET backupInfo: ${backupInfo}`);
    return backupInfo;
  }

  onProcess(): string {
    console.log(TAG, 'onProcess ok');
    let progressInfo: string = mediabackup.getProgressInfo();
    if (progressInfo.length === 0 || !this.isProgressInfoValid(progressInfo)) {
      console.error(TAG, 'progressInfo is empty or invalid, return default');
      progressInfo = JSON.stringify(DEFAULT_PROGRESS_INFO);
    }
    console.log(TAG, `GET progressInfo: ${progressInfo}`);
    return progressInfo;
  }

  private async getRestoreExInfo(sceneCode: number, restoreExResult: string): Promise<string> {
    if (!this.isRestoreExResultValid(restoreExResult)) {
      console.error(TAG, 'restoreEx result is invalid, use default');
      return JSON.stringify(DEFAULT_RESTORE_EX_INFO);
    }
    if (sceneCode !== UPGRADE_RESTORE) {
      return restoreExResult;
    }
    try {
      let jsonObject = JSON.parse(restoreExResult);
      for (let info of jsonObject.resultInfo) {
        if (info.type !== STAT_VALUE_COUNT_INFO) {
          continue;
        }
        for (let subCountInfo of info.infos) {
          let type = subCountInfo.backupInfo;
          let detailsPath = subCountInfo.details;
          subCountInfo.details = await this.getDetails(type, detailsPath);
        }
      }
      return JSON.stringify(jsonObject);
    } catch (err) {
      console.error(TAG, `getRestoreExInfo error message: ${err.message}, code: ${err.code}`);
      return JSON.stringify(DEFAULT_RESTORE_EX_INFO);
    }
  }

  private async getDetails(type: string, detailsPath: string): Promise<null | number> {
    if (detailsPath.length === 0) {
      console.log(TAG, `${type} has no failed files`);
      return null;
    }
    let file = await fs.open(detailsPath);
    console.log(TAG, `${type} details fd: ${file.fd}`);
    return file.fd;
  }

  private isRestoreExResultValid(restoreExResult: string): boolean {
    try {
      let jsonObject = JSON.parse(restoreExResult);
      if (!this.hasKey(jsonObject, STAT_KEY_RESULT_INFO)) {
        return false;
      }
      let resultInfo = jsonObject[STAT_KEY_RESULT_INFO];
      if (resultInfo.length !== RESULT_INFO_NUM) {
        console.error(TAG, `resultInfo num ${resultInfo.length} != ${RESULT_INFO_NUM}`);
        return false;
      }
      let errorInfo = resultInfo[0];
      let countInfo = resultInfo[1];
      return this.isErrorInfoValid(errorInfo) && this.isCountInfoValid(countInfo);
    } catch (err) {
      console.error(TAG, `isRestoreExResultValid error message: ${err.message}, code: ${err.code}`);
      return false;
    }
  }

  private isErrorInfoValid(errorInfo: JSON): boolean {
    if (!this.hasKey(errorInfo, STAT_KEY_TYPE) || !this.hasKey(errorInfo, STAT_KEY_ERROR_CODE) ||
      !this.hasKey(errorInfo, STAT_KEY_ERROR_INFO)) {
      return false;
    }
    if (errorInfo[STAT_KEY_TYPE] !== STAT_VALUE_ERROR_INFO) {
      console.error(TAG, `errorInfo ${errorInfo[STAT_KEY_TYPE]} != ${STAT_VALUE_ERROR_INFO}`);
      return false;
    }
    if (!this.checkType(typeof errorInfo[STAT_KEY_ERROR_CODE], JS_TYPE_STRING) ||
      !this.checkType(typeof errorInfo[STAT_KEY_ERROR_INFO], JS_TYPE_STRING)) {
      return false;
    }
    return true;
  }

  private isCountInfoValid(countInfo: JSON): boolean {
    if (!this.hasKey(countInfo, STAT_KEY_TYPE) || !this.hasKey(countInfo, STAT_KEY_INFOS)) {
      return false;
    }
    if (countInfo[STAT_KEY_TYPE] !== STAT_VALUE_COUNT_INFO) {
      console.error(TAG, `countInfo ${countInfo[STAT_KEY_TYPE]} != ${STAT_VALUE_COUNT_INFO}`);
      return false;
    }
    let subCountInfos = countInfo[STAT_KEY_INFOS];
    if (subCountInfos.length !== STAT_TYPES.length) {
      console.error(TAG, `countInfo ${subCountInfos.length} != ${STAT_TYPES.length}`);
      return false;
    }
    let hasPhoto = false;
    let hasVideo = false;
    let hasAudio = false;
    let hasLakePhoto = false;
    let hasLakeVideo = false;
    let hasFileManagerPhoto = false;
    let hasFileManagerVideo = false;
    for (let subCountInfo of subCountInfos) {
      if (!this.isSubCountInfoValid(subCountInfo)) {
        return false;
      }
      hasPhoto = hasPhoto || subCountInfo[STAT_KEY_BACKUP_INFO] === STAT_TYPE_PHOTO;
      hasVideo = hasVideo || subCountInfo[STAT_KEY_BACKUP_INFO] === STAT_TYPE_VIDEO;
      hasAudio = hasAudio || subCountInfo[STAT_KEY_BACKUP_INFO] === STAT_TYPE_AUDIO;
      hasLakePhoto = hasLakePhoto || subCountInfo[STAT_KEY_BACKUP_INFO] === STAT_TYPE_LAKE_PHOTO;
      hasLakeVideo = hasLakeVideo || subCountInfo[STAT_KEY_BACKUP_INFO] === STAT_TYPE_LAKE_VIDEO;
      hasFileManagerPhoto = hasFileManagerPhoto || subCountInfo[STAT_KEY_BACKUP_INFO] === STAT_TYPE_FILE_MANAGER_PHOTO;
      hasFileManagerVideo = hasFileManagerVideo || subCountInfo[STAT_KEY_BACKUP_INFO] === STAT_TYPE_FILE_MANAGER_VIDEO;
    }
    return hasPhoto && hasVideo && hasAudio && hasLakePhoto && hasLakeVideo &&
        hasFileManagerPhoto && hasFileManagerVideo;
  }

  private isSubCountInfoValid(subCountInfo: JSON): boolean {
    if (!this.hasKey(subCountInfo, STAT_KEY_BACKUP_INFO) || !this.hasKey(subCountInfo, STAT_KEY_SUCCESS_COUNT) ||
      !this.hasKey(subCountInfo, STAT_KEY_DUPLICATE_COUNT) || !this.hasKey(subCountInfo, STAT_KEY_FAILED_COUNT) ||
      !this.hasKey(subCountInfo, STAT_KEY_DETAILS)) {
      return false;
    }
    if (!STAT_TYPES.includes(subCountInfo[STAT_KEY_BACKUP_INFO])) {
      console.error(TAG, `SubCountInfo ${subCountInfo[STAT_KEY_BACKUP_INFO]} not in ${JSON.stringify(STAT_TYPES)}`);
      return false;
    }
    return !isNaN(subCountInfo[STAT_KEY_SUCCESS_COUNT]) && !isNaN(subCountInfo[STAT_KEY_DUPLICATE_COUNT]) &&
      !isNaN(subCountInfo[STAT_KEY_FAILED_COUNT]);
  }

  private isBackupInfoValid(backupInfo: string): boolean {
    try {
      let jsonObject = JSON.parse(backupInfo);
      let hasPhoto = false;
      let hasVideo = false;
      let hasAudio = false;
      for (let subBackupInfo of jsonObject) {
        if (!this.isSubBackupInfoValid(subBackupInfo)) {
          return false;
        }
        hasPhoto = hasPhoto || subBackupInfo[STAT_KEY_BACKUP_INFO] === STAT_TYPE_PHOTO;
        hasVideo = hasVideo || subBackupInfo[STAT_KEY_BACKUP_INFO] === STAT_TYPE_VIDEO;
        hasAudio = hasAudio || subBackupInfo[STAT_KEY_BACKUP_INFO] === STAT_TYPE_AUDIO;
      }
      return hasPhoto && hasVideo && hasAudio;
    } catch (err) {
      console.error(TAG, `isBackupInfoValid error message: ${err.message}, code: ${err.code}`);
      return false;
    }
  }

  private isSubBackupInfoValid(subBackupInfo: JSON): boolean {
    if (!this.hasKey(subBackupInfo, STAT_KEY_BACKUP_INFO) || !this.hasKey(subBackupInfo, STAT_KEY_NUMBER)) {
      return false;
    }

    return !isNaN(subBackupInfo[STAT_KEY_NUMBER]);
  }

  private hasKey(jsonObject: JSON, key: string): boolean {
    if (!(key in jsonObject)) {
      console.error(TAG, `hasKey ${key} not found`);
      return false;
    }
    return true;
  }

  private checkType(varType: string, expectedType: string): boolean {
    if (varType !== expectedType) {
      console.error(TAG, `checkType ${varType} != ${expectedType}`);
      return false;
    }
    return true;
  }

  private isProgressInfoValid(progressInfo: string): boolean {
    try {
      let jsonObject = JSON.parse(progressInfo);
      if (!this.hasKey(jsonObject, STAT_KEY_PROGRESS_INFO)) {
        return false;
      }
      let subProcessInfos = jsonObject[STAT_KEY_PROGRESS_INFO];
      for (let subProcessInfo of subProcessInfos) {
        if (!this.isSubProcessInfoValid(subProcessInfo)) {
          return false;
        }
      }
      return true;
    } catch (err) {
      console.error(TAG, `isProgressInfoValid error message: ${err.message}, code: ${err.code}`);
      return false;
    }
  }

  private isSubProcessInfoValid(subProcessInfo: JSON): boolean {
    if (!this.hasKey(subProcessInfo, STAT_KEY_NAME) || !this.hasKey(subProcessInfo, STAT_KEY_PROCESSED) ||
      !this.hasKey(subProcessInfo, STAT_KEY_TOTAL) || !this.hasKey(subProcessInfo, STAT_KEY_IS_PERCENTAGE)) {
      return false;
    }
    return !isNaN(subProcessInfo[STAT_KEY_PROCESSED]) && !isNaN(subProcessInfo[STAT_KEY_TOTAL]) &&
      this.checkType(typeof subProcessInfo[STAT_KEY_IS_PERCENTAGE], JS_TYPE_BOOLEAN);
  }

  private checkDBExist(dbPath: string): boolean {
    try {
      let res = fs.accessSync(dbPath);
      if (!res) {
        console.log(TAG, `LITE_PHONE_CLONE_RESTORE: gallery.db is not exist`);
        return false;
      }
    } catch (err) {
      console.error(TAG, `LITE_PHONE_CLONE_RESTORE: accessSync failed with error message: ` + err.message +
                    `, error code: ` + err.code);
    }
    return true;
  }

  private getSceneCode(bundleVersion: BundleVersion): number {
    if (bundleVersion.name.startsWith(UPGRADE_NAME)) {
      return UPGRADE_RESTORE;
    }
    if (bundleVersion.name === DUAL_FRAME_CLONE_NAME && bundleVersion.code === 0) {
      return this.checkDBExist(GALLERY_DB_PATH) ? DUAL_FRAME_CLONE_RESTORE : LITE_PHONE_CLONE_RESTORE;
    }
    if (bundleVersion.name === OTHERS_PHONE_FRAME_CLONE_NAME && bundleVersion.code === 0) {
      return OTHERS_PHONE_CLONE_RESTORE;
    }
    if (bundleVersion.name === I_PHONE_FRAME_CLONE_NAME && bundleVersion.code === 0) {
      return I_PHONE_CLONE_RESTORE;
    }
    if (bundleVersion.name === CLOUD_BACKUP_NAME && bundleVersion.code === 0) {
      return this.checkDBExist(GALLERY_DB_PATH) ? CLOUD_BACKUP_RESTORE : LITE_PHONE_CLONE_RESTORE;
    }
    return CLONE_RESTORE;
  }
}
