The <skeleton>s must specify all the bones that may influence vertexes of
skinned meshes. The <bone name> is the name of the bone in the relevant
modelling/animation program. The <identifier> name is used to determine
whether this <skeleton> applies to the data found in a given model file.

<target> must be the name of a bone in the standard_skeleton identified by
<skeleton target>.

The hierarchy of bones is mostly irrelevant (though it makes sense to match
the structure used by the modelling program) - the only effect is that
the default <target> (i.e. when none is specified for a given bone) is
inherited from the parent node in this hierarchy.
