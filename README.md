# SLRR Proofix
This is a proof-of-concept fix for Street Legal Racing Redline v2.3.1.

**WARNING:** It is highly experimental, not really meant to be used daily -- this is why it is **NOT** a SilentPatch.

Don't worry, none of those fixed crashes happen regularly. However, they are reproducible with memory debugging utilities enabled,
which means that during normal gameplay they can manifest themselves as random crashes, happening from time to time.

## Non-code fixes

These files don't end with a newline, which causes a buffer overrun when attempting to parse them line by line (see `testedFunction` in the source code):
- asphalt.cfg
- graph.cfg
- graph_fade.cfg
