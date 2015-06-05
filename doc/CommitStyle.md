# Commit message style #

To make the commit messages consistent, the following style of messages is recommended:
```
  [<branch>] -<type> [<#issue>] (r000): [<section>] Your message here (credits) 
```
with
  * **optional `[<branch>]`** if you commit in a separate part of the repository, for example in a branch.
  * **`-<type>`** Type of change, see the list below. **Always include this.** (and don't forget the minus-sign in front of it)
  * **optional `[<#issue>]`** if the commit solves an issue, mention it, for example `-Fix [#23]: Foo should also work with bar.`
  * **optional `(r000)`** if the commit is related to a previous commit, mention the revision.
  * **optional `[<section>]`** if the commit is in a special section, for example graphics or build system.

List of types:

  * _"Wiki"_ when you are changing the Wiki.
  * _"Add"_ when you added things
  * _"Codechange"_ when you did a change to the code only, which doesn't effect gameplay in any way (so no bugfix!)
  * _"Change"_ when you made a change (use it as little as possible, unclear entry)
  * _"Feature"_ when you added a feature
  * _"Fix"_ when you fixed something
  * _"Branch"_ when you create a branch
  * _"Remove"_ when you remove a branch/file
  * _"Revert"_ when you dare to revert something