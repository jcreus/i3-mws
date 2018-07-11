# i3-mws: 100 workspaces on i3
## What it does

<p align="center">
  <img src="demo.png">
</p>

- It extends the standard i3 workspaces to have 100 workspaces, addressable using separate mod keys: 10 "master" workspaces (controlled by one mod key) with 10 child workspaces each.
  - For instance, general purpose containers can be kept in the master workspace 1, and each major concurrent project can have its own set of 10 workspaces.
  - `$mod+#` switches to child workspace `#` under the current master workspace (same as in normal i3 operation).
  - `$mod2+#` switches to master workspace `#`, remembering the last child workspace used in that master workspace.
  - `$mod+Shift+#` moves the current container to child workspace `#`, also much like standard i3.
  - `$mod2+Shift+#` moves the current container to master workspace `#`, preserving the child workspace number (probably rare in practice).
- Whenever an external monitor gets connected or disconnected, it remembers what output each workspace was in, and moves it back when it gets reconnected.
  - Due to quirks in the author's implementation of multi-monitor, this _might_ need tweaking to work on other systems, but is probably fine, and can be disabled.
  
## Installation
Only required library is `json-c`, but it was picked because it seemingly was already installed in the system, including header files, so odds are nothing needs to be installed.

Steps:
1. Clone the repo.
2. If using the option to remember workspace location in multi-monitor setups (recommended, if it works), uncomment `#define MULTI_SCREEN` at the top of `mws-server.c`, and set the name of your xrandr outputs in the `WORKSPACES` array.
3. `make && make install`
4. Modify your i3 config, as in the file `i3_config_changes`. Remember to remove or comment out the existing `bindsym`s that deal with workspaces. Remember to add `strip_workspace_numbers yes` to the `bar {}` config, besides the key bindings.
5. Restart i3 or, for good measure, reboot/exit and log back in.

## Internals
Hopefully this is not necessary to get it to work, but in case it helps with debugging:
- If using named workspaces, i3 only sorts based on the first character, and thus the workspaces would appear out of order. i3-wms has a numbered prefix followed by the actual string; for instance, workspace `2.1` is actually `21:2.1` internally.
- To minimize latency—unclear if it would actually matter—it's written in C, and it uses a UNIX socket started by `mws-server` that `mws-client` (called by the keybindings) talks to. `mws-server` additionally talks to the i3 IPC server to receive notifications of user-changed workspace focus (e.g. scrolling on the status bar). The implications of this, together with the fact it was meant to be a quick script, are that the code doesn't look to great.
