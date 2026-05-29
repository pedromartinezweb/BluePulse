# SignPath setup

BluePulse is prepared for SignPath Foundation code signing through GitHub Actions.

## Repository settings

After the SignPath Foundation application is approved, create these GitHub Actions variables:

- `SIGNPATH_ORGANIZATION_ID`
- `SIGNPATH_PROJECT_SLUG`
- `SIGNPATH_SIGNING_POLICY_SLUG`

Create this GitHub Actions secret:

- `SIGNPATH_API_TOKEN`

GitHub path:

```text
Settings -> Secrets and variables -> Actions
```

## Expected SignPath project settings

Use these values unless SignPath provides different names:

- Project slug: `bluepulse`
- Signing policy slug: `release-signing`
- Artifact name: `BluePulse.exe`
- Artifact type: Windows executable signed with Authenticode
- Build system: GitHub Actions
- Repository: `https://github.com/pedromartinezweb/BluePulse`
- Release branch: `main`

## Release flow

The workflow does the following:

1. Builds `dist\BluePulse.exe` on a GitHub-hosted Windows runner.
2. Uploads the unsigned executable as a workflow artifact.
3. Submits that artifact to SignPath.
4. Waits for the signed executable.
5. Verifies the Authenticode signature with `signtool`.
6. Publishes the signed executable to GitHub Releases.

If SignPath is not configured, the workflow stops after uploading the unsigned workflow artifact and does not publish a GitHub Release.
