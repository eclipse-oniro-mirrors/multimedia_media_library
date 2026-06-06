# Context: Multimedia Media Library

## Glossary

| Term | Definition |
|------|-----------|
| **Docs Media Scan** | A one-time DFX task that traverses `/storage/media/local/files/Docs` to collect media statistics from leaf folders. Triggered during charging+screen-off background operations, only on beta devices. |
| **Leaf Folder** | A directory that directly contains at least one media file (IMAGE or VIDEO). Directories that only contain media files in their subdirectories are not leaf folders and are not individually统计. |
| **Media File** | A file recognized as IMAGE or VIDEO by either `MediaFileUtils::GetMediaType` or `MediaFileUtils::GetMediaTypeNotSupported`. AUDIO files are NOT considered media files for this task. Detection priority: `GetMediaType` first; if not IMAGE/VIDEO, then `GetMediaTypeNotSupported`. |
| **Docs Scan Temp Table** | An RDB temporary table (`docs_media_scan_temp`) created during traversal to store per-folder statistics and track traversal progress. Lifetime: CREATE at start of traversal → DROP after full reporting completes. Retained across interruptions for resumption. |
| **Docs Scan Phases** | Three lifecycle phases: (1) **Traversal** — recursively scan leaf folders, INSERT stats into temp table; (2) **Reporting** — aggregate from temp table, batch-report via HiSysEvent; (3) **Finalize** — mark prefs as done, DROP temp table. |
| **Size Distribution Buckets** | Seven mutually exclusive intervals using 1024-byte units: ≤1k [0,1024], ≤10k (1024,10240], ≤50k (10240,51200], ≤100k (51200,102400], ≤500k (102400,512000], ≤1m (512000,1048576], >1m (1048576,∞). Represented as a fixed 7-element array in order, no keys. |
| **atime Range** | The difference between max(st_atime) and min(st_atime) of media files within a leaf folder, measured in seconds. Reported as both a boolean (≤30min = ≤1800sec) and the raw diff in seconds. Uses `lstat`; symlink files are skipped. |
| **LOAD_TYPE** | Enum distinguishing report types within the `MEDIALIB_ANCO_COUNT_FORMAT_INFO` event. Values: LAKE_FIRST_LOAD(0), FILEMANAGER_FIRST_LOAD(1), FILEMANAGER_CLONE_FIRST_LOAD(2), DOCS_MEDIA_SCAN(3). |
| **Daily Report Limit** | Self-managed limit of 80 HiSysEvent calls per day for Docs Media Scan. Tracked via preferences keys (`docs_media_scan_daily_count` and `docs_media_scan_daily_date`), reset on date change. |

## Key Paths

| Path | Purpose |
|------|---------|
| `/storage/media/local/files/Docs` | Root directory for traversal. If not present, task is marked as completed immediately. |
| `/data/storage/el2/base/preferences/dfx_common.xml` | Preferences file for Docs Scan state tracking (completion flags, progress counters, daily limits). NOT `task_progress.xml`. |

## Preferences Keys (dfx_common.xml)

| Key | Type | Default | Meaning |
|-----|------|---------|---------|
| `docs_media_scan_done` | bool | false | Overall task completed. true = never run again. |
| `docs_media_scan_traversal_done` | bool | false | Traversal phase completed. All leaf folders scanned. |
| `docs_media_scan_reported_folders` | int | 0 | Number of folders successfully reported so far. Used for batch offset. |
| `docs_media_scan_daily_count` | int | 0 | HiSysEvent calls made today for this scan. |
| `docs_media_scan_daily_date` | string | "" | Date string (e.g. "20260606") for daily count reset. |

## Temp Table Schema (docs_media_scan_temp)

| Column | Type | Purpose |
|--------|------|---------|
| `id` | INTEGER PK AUTOINCREMENT | Row ID |
| `dir_path` | TEXT NOT NULL UNIQUE | Relative path from Docs root (e.g. "Screenshots") |
| `image_count` | INTEGER DEFAULT 0 | Number of IMAGE files in this folder |
| `video_count` | INTEGER DEFAULT 0 | Number of VIDEO files in this folder |
| `format_distribution` | TEXT DEFAULT '{}' | JSON map: {"jpg":3, "png":2} — lowercase extensions → count |
| `size_distribution` | TEXT DEFAULT '[]' | JSON 7-element array: [0,1,3,2,0,1,0] per bucket order |
| `atime_within_30min` | INTEGER DEFAULT 0 | 0=false, 1=true — atime range ≤ 1800 seconds |
| `atime_diff_sec` | INTEGER DEFAULT 0 | Raw atime diff in seconds (max_atime - min_atime) |

## Per-Folder JSON Structure (short keys)

```json
{
  "dp": "Screenshots",       // dir_path (relative)
  "ic": 5,                   // image_count
  "vc": 2,                   // video_count
  "fd": {"jpg": 3, "png": 2, "mp4": 2},  // format_distribution
  "sd": [0, 1, 3, 2, 0, 1, 0],           // size_distribution (7-element array)
  "a30": true,                // atime_within_30min
  "as": 1200                  // atime_diff_sec
}
```

## HiSysEvent Reporting

| Parameter | Value for Docs Scan |
|-----------|---------------------|
| Event Name | `MEDIALIB_ANCO_COUNT_FORMAT_INFO` (reuse existing) |
| Domain | `MEDIALIBRARY` |
| EventType | `STATISTIC` |
| Function | `DfxReporter::ReportAncoCountFormatInfoForDirScan` (new) |
| LOAD_TYPE | 3 (DOCS_MEDIA_SCAN) |
| ALBUM_COUNT | 0 |
| IMAGE_COUNT | 0 |
| VIDEO_COUNT | 0 |
| ASSET_FORMAT_DISTRIBUTION | JSON array of per-folder objects, 20 folders per event |

## Traversal Rules

- **Hidden directories**: NOT skipped (scanned normally)
- **Symbolic links**: Detected via `lstat`, skipped (not followed)
- **Traversal API**: `opendir/readdir/lstat` (POSIX), consistent with `MediaFileInterworkScanner`
- **Interruption**: Check `MedialibrarySubscriber::IsCurrentStatusOn()` after each directory
- **Beta check**: `IsBetaVersion()` — non-beta devices skip entirely
- **Resumption**: Temp table preserves progress; next run skips folders already in table

## Architectural Placement

- **Trigger**: Direct call in `MedialibrarySubscriber::DoBackgroundOperationStepTwo()` at the end
- **Core logic**: `DocsMediaScanManager` (singleton, new class)
- **Data access**: Temp table operations in `DfxDatabaseUtils` (new methods)
- **Reporting**: `DfxReporter::ReportAncoCountFormatInfoForDirScan` (new method)