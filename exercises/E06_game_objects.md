# Exercise 06 - Game Objects
This week's exercise will be taking a bit longer the capstone of the first part of the course. Your objective is to select one of the previous exercises (hint: the earlier, the easier) and re-implement them using a GameObject/Component structure and/or serialization. You can check the slides and the book for references, and you can do it as simple or as tight as you seem fit.

You will have a bit more time than usual given the autumn break (but don't forget to rest a bit too!) and you are free to do it in group. Next lecture, whoever wants can present their solution and we can discuss them togheter during the lab. session.

## 06.1 GameObject/Component
What we would like out GameObject/Component structure to do:
- create a hierarchy of gameplay entities (GameObjects) that can have arbitrary data and behaviour (Components)
- store references to other GameObjects
- adding and removing GameObjects and Components freely
- allow for adding new types of Components easily

A few questions that you will probably need to answer while working on the exerciseL
- how do you integrate your structure with SDL, itu_engine and/or the previous lectures code?
  (If you have troubles, you can start by keeping the context as a global variable, not the best solution but should work for the exercise)
- how do you create/destroy object during update? Doing it immediately will create a lot of problems...
- how do you store references to other GameObjects, making it sure you can detect if they are still valid?

Additional challenges for a more robust implementation:
- try to minimize the amount of global/static context
- make it so user-facing code (ie, specific Component types) can't directly create/destroy objects, but is forced (as much as possible) to go through safe paths
- load/store an entire hierarchy from file (see ex 06.2)

## 06.2 Serialization
Look at the exercise you are re-implementing and identify what parts of the code are good candidates to be converted to reading from a file rather than be hardcoded.
- during the lecture we pointed at the most obvious ones, can you find more?
- can you apply those principles to your GameObject/Component structure?