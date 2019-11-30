from paraview import simple
import os, json

# -----------------------------------------------------------------------------

def getAllNames():
    actorNameMapping = {}
    srcs = simple.GetSources()
    duplicates = {}
    for key, val in srcs.items():
        # Prevent name duplication
        nameToUse = key[0]
        if nameToUse in duplicates:
            count = 1
            newName = '%s (%d)' % (nameToUse, count)
            while newName in duplicates:
                count += 1
                newName = '%s (%d)' % (nameToUse, count)
            nameToUse = newName
        duplicates[nameToUse] = True
        actorRep = simple.GetRepresentation(val).GetClientSideObject().GetActiveRepresentation().GetActor()
        actorNameMapping[nameToUse] = actorRep
    return actorNameMapping


# -----------------------------------------------------------------------------

def findName(names, actor, defaultName):
    for name in names:
        if actor == names[name]:
            return name
    return defaultName


# -----------------------------------------------------------------------------

def getRenameMap():
    renameMap = {}
    names = getAllNames()
    view = simple.GetActiveView()
    renderer = view.GetClientSideObject().GetRenderer()
    viewProps = renderer.GetViewProps()
    idx = 1
    for viewProp in viewProps:
        if not viewProp.GetVisibility():
            continue
        if not viewProp.IsA('vtkActor'):
            continue
        bounds = viewProp.GetBounds()
        if bounds[0] > bounds[1]:
            continue
        # The mapping will fail for multiblock that are composed of several blocks
        # Merge block should be used to solve the renaming issue for now
        # as the id is based on the a valid block vs representation.
        strIdx = '%s' % idx
        renameMap[strIdx] = findName(names, viewProp, strIdx)
        idx += 1
    return renameMap


# -----------------------------------------------------------------------------

def applyParaViewNaming(directoryPath):
    renameMap = getRenameMap()
    scene = None
    filePath = os.path.join(directoryPath, 'index.json')
    with open(filePath) as file:
        scene = json.load(file)
        for item in scene['scene']:
            if item['name'] in renameMap:
                item['name'] = renameMap[item['name']]

    with open(filePath, 'w') as file:
        file.write(json.dumps(scene, indent=2))


# -----------------------------------------------------------------------------

def addViewPoints(directoryPath, viewPoints):
    # The viewPoints is a dict structured like so:
    #
    # name1: {
    #     'camera': vtkCamera,
    #     'show': [ repProxy1, repProxy2, ... ],
    #     'hide': [ repProxy3, ...]
    # },
    # name2: { ... }

    sourceNames = getAllNames()

    # These are data sets that will not be written out. Do not
    # include them in the hide list.
    doNotHideList = [name for name, actor in sourceNames.items()
                     if not actor.GetVisibility()]

    cameraViewPoints = {}
    for name, viewPoint in viewPoints.items():
        # Convert the representation proxies into data set names
        # This also takes into account the name duplication renaming strategy
        show = [x.GetClientSideObject().GetActiveRepresentation().GetActor()
                for x in viewPoint['show']]
        hide = [x.GetClientSideObject().GetActiveRepresentation().GetActor()
                for x in viewPoint['hide']]

        show = [findName(sourceNames, x, '') for x in show]
        hide = [findName(sourceNames, x, '') for x in hide]

        hide = [x for x in hide if x not in doNotHideList]

        camera = viewPoint['camera']
        cameraViewPoints[name] = {
            'camera': {
                'focalPoint': camera.GetFocalPoint(),
                'position': camera.GetPosition(),
                'viewUp': camera.GetViewUp()
            },
            'show': show,
            'hide': hide
        }

    filePath = os.path.join(directoryPath, 'index.json')
    with open(filePath) as file:
        root = json.load(file)
        root["cameraViewPoints"] = cameraViewPoints

    with open(filePath, 'w') as file:
        file.write(json.dumps(root, indent=2))
