# Commit message style #

To make the commit messages consistent, the following style of messages is recommended:
```
  <type>: [<section>] Your message here [Fix #<num>] (credits) ```
with
  * **`<type>`** Type of change, see the list below. **Always include this.**
  * **optional `[<section>]`** if the commit is in a special section, for example graphics or build system.
  * **optional `[Fix #<num>]`** if the commit solves an issue, mention it. This means that a future reader can find the issue to see what the cause for the fix was. Also, the issue integration on Codeberg will automatically close said issue if it is included in the commit message.

List of types:

  * _"Add"_ when you added things
  * _"Codechange"_ when you did a change to the code only, which doesn't effect gameplay in any way (so no bugfix!)
  * _"Change"_ when you made a change (use it as little as possible, unclear entry)
  * _"Cleanup" when you are removing some old code that is no longer needed/used
  * _"Feature"_ when you added a feature
  * _"Fix"_ when you fixed something
  * _"Revert"_ when you dare to revert something
