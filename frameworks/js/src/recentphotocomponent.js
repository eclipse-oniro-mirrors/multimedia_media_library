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

if (!('finalizeConstruction' in ViewPU.prototype)) {
    Reflect.set(ViewPU.prototype, 'finalizeConstruction', () => { });
}

const photoAccessHelper = requireNapi('file.photoAccessHelper');
const BaseItemInfo = requireNapi('file.PhotoPickerComponent').BaseItemInfo;

const FILTER_MEDIA_TYPE_ALL = 'FILTER_MEDIA_TYPE_ALL';
const FILTER_MEDIA_TYPE_IMAGE = 'FILTER_MEDIA_TYPE_IMAGE';
const FILTER_MEDIA_TYPE_VIDEO = 'FILTER_MEDIA_TYPE_VIDEO';
const FILTER_MEDIA_TYPE_IMAGE_MOVING_PHOTO = 'FILTER_MEDIA_TYPE_IMAGE_MOVING_PHOTO';
const SECONDS_OF_ONE_DAY = 24 * 60 * 60;
const RECENT_PHOTO_PERIOD_INT_MAX = 2147483647;

const OperationType = {
    EQUAL_TO : 1,
    NOT_EQUAL_TO : 2,
    GREATER_THAN : 3,
    LESS_THAN : 4,
    GREATER_THAN_OR_EQUAL_TO : 5,
    LESS_THAN_OR_EQUAL_TO : 6,
    AND : 7,
    OR : 8,
    IN : 9,
    NOT_IN : 10,
    BEGIN_WRAP : 11,
    END_WRAP : 12,
    BETWEEN : 13,
    NOT_BETWEEN : 14
};

const PickerFilterPhotoKeys = {

    URI: 'uri',

    PHOTO_TYPE: 'media_type',

    DISPLAY_NAME: 'display_name',

    SIZE: 'size',

    DURATION: 'duration',

    WIDTH: 'width',

    HEIGHT: 'height',

    ORIENTATION: 'orientation',

    FAVORITE: 'is_favorite',

    TITLE: 'title',

    POSITION: 'position',

    PHOTO_SUBTYPE: 'subtype',

    DYNAMIC_RANGE_TYPE: 'dynamic_range_type',

    COVER_POSITION: 'cover_position',

    BURST_KEY: 'burst_key',

    LCD_SIZE: 'lcd_size',

    THM_SIZE: 'thm_size',

    DETAIL_TIME: 'detail_time',

    OWNER_ALBUM_ID: 'owner_album_id',

    MEDIA_SUFFIX: 'media_suffix',

    ASPECT_RATIO: 'aspect_ratio',

    DATE_TAKEN_MS: 'date_taken_ms',
};

export class RecentPhotoComponent extends ViewPU {
    constructor(j3, k3, l3, m3 = -1, n3 = undefined, o3) {
        super(j3, l3, m3, o3);
        if (typeof n3 === 'function') {
            this.paramsGenerator_ = n3;
        }
        this.recentPhotoOptions = undefined;
        this.onRecentPhotoCheckResult = undefined;
        this.onRecentPhotoClick = undefined;
        this.onRecentPhotoCheckInfo = undefined;
        this.onError = undefined;
        this.setInitiallyProvidedValue(k3);
        this.finalizeConstruction();
    }
    setInitiallyProvidedValue(i3) {
        if (i3.recentPhotoOptions !== undefined) {
            this.recentPhotoOptions = i3.recentPhotoOptions;
        }
        if (i3.onRecentPhotoCheckResult !== undefined) {
            this.onRecentPhotoCheckResult = i3.onRecentPhotoCheckResult;
        }
        if (i3.onRecentPhotoClick !== undefined) {
            this.onRecentPhotoClick = i3.onRecentPhotoClick;
        }
        if (i3.onRecentPhotoCheckInfo !== undefined) {
            this.onRecentPhotoCheckInfo = i3.onRecentPhotoCheckInfo;
        }
        if (i3.onError !== undefined) {
            this.onError = i3.onError;
        }
    }
    updateStateVars(h3) {
    }
    purgeVariableDependenciesOnElmtId(g3) {
    }

    checkAssetFilterInvalid(assetFilter) {
        // 获取所有有效的值
        const validOperationTypes = Object.values(OperationType);
        const validPhotoKeys = Object.values(PickerFilterPhotoKeys);

        //遍历数组中的每个OperationItem
        for (const item of assetFilter) {
            //检查operationType是否有值且在枚举中
            if (!item || !item.operationType || !validOperationTypes.includes(item.operationType)) {
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
                if (item.field === PickerFilterPhotoKeys.URI && item.operationType !== OperationType.EQUAL_TO) {
                    console.log('PhotoPickerComponent, Invalid uri operation');
                    return true;
                }
            }
        }
        return false;
    }
    aboutToBeDeleted() {
        SubscriberManager.Get().delete(this.id__());
        this.aboutToBeDeletedInternal();
    }

    validateAssetFilter() {
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

    initialRender() {
        if (!this.validateAssetFilter()) {
            return;
        }
        this.observeComponentCreation2((e3, f3) => {
            Row.create();
            Row.height('100%');
        }, Row);
        this.observeComponentCreation2((c3, d3) => {
            Column.create();
            Column.width('100%');
        }, Column);
        this.observeComponentCreation2((v2, w2) => {
            SecurityUIExtensionComponent.create({
                bundleName: 'com.ohos.photos',
                abilityName: 'RecentUIExtensionAbility',
                parameters: {
                    'ability.want.params.uiExtensionType': 'recentPhoto',
                    filterMediaType: this.convertMIMETypeToFilterType(this.recentPhotoOptions?.MIMEType),
                    period: this.normalizePeriod(this.recentPhotoOptions?.period),
                    photoSource: this.recentPhotoOptions?.photoSource,
                    isAutoRefreshSupported: this.recentPhotoOptions?.isAutoRefreshSupported,
                    colorMode: this.recentPhotoOptions?.colorMode,
                    isFromPickerView: true,
                    isRecentPhotoCheckResultSet: this.onRecentPhotoCheckResult ? true : false,
                    isPersistAfterClick: this.recentPhotoOptions?.isPersistAfterClick,
                    isShowBadge: this.recentPhotoOptions?.isShowBadge,
                    assetFilter: this.recentPhotoOptions?.assetFilter,
                }
            });
            SecurityUIExtensionComponent.height('100%');
            SecurityUIExtensionComponent.width('100%');
            SecurityUIExtensionComponent.onRemoteReady(() => {
                console.info('RecentPhotoComponent onRemoteReady');
            });
            SecurityUIExtensionComponent.onReceive((a3) => {
                let b3 = a3;
                this.handleOnReceive(b3);
            });
            SecurityUIExtensionComponent.onError((error) => {
                this.handleOnError(error);
            });
        }, SecurityUIExtensionComponent);
        Column.pop();
        Row.pop();
    }

    handleOnError(error) {
        console.info('RecentPhotoComponent onError: ' + JSON.stringify(error));
        if (this.onError) {
            let e = new PickerError();
            e.functionName = error?.name || 'extension_exit_abnormally';
            e.errorCode = error?.code ?? 0;
            e.message = error?.message || 'the extension ability exited abnormally, please check the log.';
            this.onError(e);
        }
    }

    handleOnReceive(p2) {
        console.info('RecentPhotoComponent OnReceive:' + this.encrypt(JSON.stringify(p2)));
        let q2 = p2.dataType;
        if (q2 === 'checkResult') {
            if (this.onRecentPhotoCheckResult) {
                this.onRecentPhotoCheckResult(p2.isExist);
            }
        }
        else if (q2 === 'select') {
            if (this.onRecentPhotoClick) {
                let r2 = new BaseItemInfo();
                r2.uri = p2.uri;
                r2.mimeType = p2.mimeType;
                r2.width = p2.width;
                r2.height = p2.height;
                r2.size = p2.size;
                r2.duration = p2.duration;
                this.onRecentPhotoClick(r2);
            }
            else {
                console.warn('RecentPhotoComponent onReceive data type is invalid.');
            }
        }
        else if (q2 === 'checkInfo') {
            if (this.onRecentPhotoCheckInfo) {
                let s2 = new RecentPhotoInfo();
                s2.identifier = p2.identifier;
                s2.dateTaken = p2.dateTaken;
                this.onRecentPhotoCheckInfo(p2.isExist, s2);
            }
        }
    }
    normalizePeriod(period) {
        if (period === Number.POSITIVE_INFINITY) {
            return RECENT_PHOTO_PERIOD_INT_MAX;
        }
        if (typeof period !== 'number' || Number.isNaN(period) || period <= 0 ||
            period > RECENT_PHOTO_PERIOD_INT_MAX) {
            return SECONDS_OF_ONE_DAY;
        }
        return period;
    }
    convertMIMETypeToFilterType(e) {
        let o;
        if (e === photoAccessHelper.PhotoViewMIMETypes.IMAGE_TYPE) {
            o = FILTER_MEDIA_TYPE_IMAGE;
        } else if (e === photoAccessHelper.PhotoViewMIMETypes.VIDEO_TYPE) {
            o = FILTER_MEDIA_TYPE_VIDEO;
        } else if (e === photoAccessHelper.PhotoViewMIMETypes.MOVING_PHOTO_IMAGE_TYPE) {
            o = FILTER_MEDIA_TYPE_IMAGE_MOVING_PHOTO;
        } else {
            o = FILTER_MEDIA_TYPE_ALL;
        }
        console.info('RecentPhotoComponent convertMIMETypeToFilterType: ' + JSON.stringify(o));
        return o;
    }
    rerender() {
        this.updateDirtyElements();
    }
    encrypt(data) {
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

export class RecentPhotoOptions {
}

export class RecentPhotoInfo {
}

export class PickerError {

}

export let PhotoSource;
(function (m2) {
    m2[m2.ALL = 0] = 'ALL';
    m2[m2.CAMERA = 1] = 'CAMERA';
    m2[m2.SCREENSHOT = 2] = 'SCREENSHOT';
})(PhotoSource || (PhotoSource = {}));

export default { RecentPhotoComponent, RecentPhotoOptions, PhotoSource, RecentPhotoInfo, PickerError };
