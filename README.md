# rialto

## Project webpage (with current coverage report and Doxygen)
https://rdkcentral.github.io/rialto/

## Coding Guidelines:
https://wiki.rdkcentral.com/display/ASP/Rialto+Coding+Guidelines

## Commit message template:
https://wiki.rdkcentral.com/display/ASP/Commit+message+template

## Building and Running Unit Tests:
https://wiki.rdkcentral.com/display/ASP/Unit+Tests

## Cross-repository PR checks

The [OCDM](.github/workflows/rialto-ocdm-test.yml) and
[GStreamer](.github/workflows/rialto-gstreamer-test.yml) checks first look for a
matching feature or target branch, then use the platform manifest fallback if
neither exists. Lookup errors fail the check; only absent branches permit fallback.

Both paths use public headers checked out at the actual Rialto PR head commit,
including fork PRs. Headers are copied before building without the clean option
so the adapter downloader cannot replace them. The required `build-and-test`
check succeeds only when dependency selection and the selected tests succeed.
Manual runs remain available from `master` only. They use a separate job with
fixed adapter commits from merged Master history, not feature branches or
manifest-selected code. GStreamer uses `e4325dd947928542017b820c29b3dcf9bc80edef`;
OCDM and GStreamer metadata use `990e56634478bcb3d86c4e5ef18e66ce787f1245`.
Updating these pins requires reviewing the new revisions and their compatibility.
The dispatched Rialto Master commit supplies the headers. Protect `master` from
unreviewed changes; these jobs rely on that branch being trusted. Dispatching
from another branch or tag fails the required check explicitly.

PR builds retain dynamic dependency selection and use the actual PR head
repository and commit for headers. They cannot enter the manual build job.

## Building Rialto using Yocto:
https://wiki.rdkcentral.com/display/ASP/Building+Rialto+using+Yocto

## Running Rialto:
https://wiki.rdkcentral.com/display/ASP/Running+YouTube+with+Rialto
