# Media Library

The Huawei multimedia media library — on-device storage, querying, and audit
of photo/video/audio assets, including cloud sync and background cleanup.

## Language

**MediaType**:
The classification of a media asset — `MEDIA_TYPE_IMAGE`, `MEDIA_TYPE_VIDEO`,
`MEDIA_TYPE_AUDIO`, etc. Stored on assets as an `int32_t` enum.
_Avoid_: "file type", "kind".

**mediaType** (audit field):
The human-readable **string name** of a `MediaType` (e.g. `"IMAGE"`, `"VIDEO"`,
`"AUDIO"`, `"PHOTO"`), written as a column on delete audit records. Kept
**alongside** the existing `int32_t type` column, which it disambiguates.
Emitted as the enum member name **without** the `MEDIA_TYPE_` prefix;
out-of-range values become `"UNKNOWN"`. `MEDIA_TYPE_PHOTO` and
`MEDIA_TYPE_IMAGE` remain distinct strings (both occur on the photos table).
_Avoid_: using the bare int `type` column to mean media type (it is overloaded).

**AuditLog**:
A single record appended to the audit CSV (`media_library_audit.csv`)
capturing one media-library operation — `ADD`, `DELETE`, `CLOUD_DOWNLOAD`,
`CLOUD_SYNC_ALBUM`, `CLOUD_EXIT`, etc. Serialized uniformly: `Write()` emits
every field; callers populate only the fields relevant to their operation.

**DfxType**:
The delete **scenario** being audited — e.g. `TRASH_PHOTO`,
`ALBUM_DELETE_ASSETS`, `DELETE_LOCAL_ASSETS_PERMANENTLY`,
`ALBUM_REMOVE_PHOTOS`. An `int32_t` enum, distinct from **MediaType**.

**operationType**:
The action being audited, as a string — e.g. `"ADD"`, `"DELETE"`,
`"CLOUD_DOWNLOAD"`. This is the field checked to decide "is this a delete".

## Relationships

- An **AuditLog** has an **operationType** (the action) and, for media
  deletes, a **mediaType** (the string name of the asset's **MediaType**).
- **mediaType** is the string form of the **MediaType** enum.

## Example dialogue

> **Dev:** "When we log a DELETE, which column tells us the deleted asset's
>  media type?"
> **Domain expert:** "That's `mediaType='IMAGE'`. Don't infer it from the
>  int `type` column — on the user-behavior delete path `type` holds the
>  `DfxType` scenario (e.g. `TRASH_PHOTO`), and on album deletes it holds the
>  album subtype. Neither is a media type."

## Flagged ambiguities

- The `AuditLog.type` int column is a **generic int bucket, overloaded across
  semantics**: it carries a **DfxType** (delete scenario) on the user-behavior
  delete path (`dfx_manager`), the asset's **MediaType** on the offline
  photo-cleanup path, the **album subtype** on the offline album-cleanup
  path, the download type on cloud downloads, and the album type on cloud
  sync albums. Resolved by introducing the dedicated `mediaType` string field
  to carry the asset's real media type (the string name of `MediaType`) on
  media deletes. Album deletes are not media assets and must not populate
  `mediaType`.
- On the user-behavior delete path (`dfx_manager`), the asset's `MediaType`
  is **not currently recorded at all** — `type` holds the `DfxType` scenario.
  `mediaType` therefore adds genuinely new information there, and must be
  sourced (from the caller/URI/DB) rather than derived from the existing
  `type` column.
