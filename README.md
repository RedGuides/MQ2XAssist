# MQ2XAssist

This plugin adds a couple of options to the /xtarget command.

The most important being the ability to add mobs by their ID instead of just their name.

## Instructions

Load the plugin by typing /plugin MQ2XAssist

| Command                           | Description                                                          |
|:----------------------------------|:---------------------------------------------------------------------|
| /xtarget                          | Display commands                                                     |
| /xtarget set <slot> <player_name> | Assign a player or NPC to a slot. Example: <br> /xtarget set 3 Ladon |
| /xtarget set <slot> <role>        | Assign a role to a slot. Example: <br> /xtarget set 3 grouptank      |
| /xtarget show on/off              | Toggle the Extended Targets window                                   |
| /xtarget target <slot>            | Select a target at a specific slot. Example: <br> /xtarget 7         |
| /xtarget auto on/off              | Toggle auto-hater-targeting. Example: <br> /xtarget auto on          |
| /xtarget add                      | will add current target to your next available slot                  |
| /xtarget remove                   | will remove the last added target                                    |
| /xtarget remove target            | will remove targeted player from slots not using rolls               |

## TLO

XAssist.XTFullHaterCount - returns all auto haters including the one that is being targeted

XAssist.XTXAggroCount[#] - allows the aggro range to be expanded to 1000 for situations where someone wants to check the >=100 aggro values. # is the aggro value provided.