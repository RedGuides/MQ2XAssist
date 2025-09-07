---
tags:
  - command
---

# /xtarget

## Syntax

<!--cmd-syntax-start-->
```eqcommand
/xtarget id <#> [<slot>] | assist <#>
```
<!--cmd-syntax-end-->

## Description

<!--cmd-desc-start-->
MQ2XAssist extends the EverQuest `/xtarget` command with two options: add spawns by ID, or when another player fights them. Common abbreviation: <span class=accent>/xtar</span>
<!--cmd-desc-end-->

## Options

**id** `<#id>` `[<#slot>]`
:   Add spawn ID to a specific xtarget slot. Slot number is optional.

**assist** `[<#id>]`
:   Will place mobs that the specified player ID fights on XTarget. `/xtarget assist` without an ID will erase the setting.

## Examples

!!! example "`/xtarget assist ${Target.ID}` - Will place mobs your current target fights on Xtarget"

!!! example "`/xtarget id 7056` - Add the mob you have targeted to XTarget"

## See also

- [/rtarget](../everquest/commands/cmd-rtarget.md)
- [/target](../everquest/commands/cmd-target.md)