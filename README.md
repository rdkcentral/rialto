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
Manual runs remain available.

## Building Rialto using Yocto:
https://wiki.rdkcentral.com/display/ASP/Building+Rialto+using+Yocto

## Running Rialto:
https://wiki.rdkcentral.com/display/ASP/Running+YouTube+with+Rialto
