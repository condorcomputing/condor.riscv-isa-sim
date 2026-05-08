## Branch model

`spike_stf` is the stable public branch for users.

The original public repository history, which was initialized from a source snapshot rather than as a true fork of upstream Spike, is preserved at `legacy/original-public-history`.

Condor STF changes are maintained as linear patch stacks on top of selected upstream Spike commits. Permanent patch-stack branches use the form:

    patch-stack/spike-stf-YYYY-MM-DD-upstream-<hash>

where `YYYY-MM-DD` is the commit date of the upstream Spike base revision and `<hash>` is the corresponding upstream Spike commit hash. These permanent patch-stack branches are not force-pushed.

For each permanent patch-stack branch, `spike_stf` contains a corresponding integration commit. The tree of the integration commit is identical to the tip of the patch-stack branch. The patch-stack branch is the reviewable form of the changes; `spike_stf` is the stable branch users should track.

Active development may happen on:

    patch-stack/spike-stf-devel

This branch is mutable and may be rebased or force-pushed before being promoted to a permanent patch-stack branch.
