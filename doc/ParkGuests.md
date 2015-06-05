How guests interact with the park defines gameplay, sets the challenges of the game and controls the game difficulty.

Each guest has several properties which defined their behaviour; character traits, preferences, etc. They also have several properties; happiness, hunger, anger, etc. The parks scenery, rides, food, etc. Affect the guests properties depending on their preferences. In turn the guests properties affect their behaviour; willingness to spend money, ride rides, vandalise scenery, etc.

## Guest preferences ##
  * Ride intensity
  * Nausea tolerance
  * Willingness to spend money
  * (Dietary preferences (salt, sweet, etc.)?)

## Guest properties ##
  * Happiness (from happy to angry)
  * Energy (from energetic to tired)
  * Nausea
  * Amount of money to spend
  * Hunger
  * Thirst
  * Toilet

## Park impacts ##
### Rides ###
Guests choose to ride rides based on their ride intensity preference, ride excitingness and ride nausea rating. They also consider how much the ride costs with respect to their willingness to spend money.
  * Increase happiness (modulated by ride intensity and guest preference and ride excitement)
  * Increase nausea (modulated by ride nausea rating)

### Scenery ###
Scenery around paths and rides has two effects:
  * Improves happiness
  * Modulates ride excitement

### Food and drink ###
Food and drink items can be bought and consumed, see ParkGuestItems.
  * Happiness (modulated by dietary preference)
  * Hunger level (food reduces)
  * Thirst level (drinks reduce, salty food increases)
  * (Nausea level (food increases)?)
  * Toilet (increases)

### Navigation ###
  * Getting lost increases anger
  * Walking decreases energy (tired guest walk slower)
  * Litter and vandalism decrease happiness

### Property interaction ###
Various guest properties impact each other, e.g.:
  * Desire for food and drink modulated by guest nausea

### Properties and behaviours ###
  * Likelihood of being sick increases with nausea
  * Willingness to spend money modulated by happiness /anger
  * Willingness to vandalise depends on anger

## Graphics ##
Most guests properties do not have visible effects on the guest sprites. The exceptions are:
  * High nausea: Face sprite changed to a green one.
  * High anger: Face sprite changed to a red one
  * Low energy: Body sprite changed to a slouched one with slower movement.
  * High energy: guest movement sped up with a corresponding increase with animation frame rate.

## Interaction ##
Most guest info is accessed by either clicking on the guest to open the guest's individual info window, or selecting that guest from the guest list to open the info window.