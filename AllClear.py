# script used to remove files that have been generated by previous test runs of the code. Used for tests
# but could be used when code is further developed.
from pathlib import Path
from HelperMethods import getProjectFolder

########################################################################################################################
TAG = "AllClear: "
print(TAG + "Running AllClear.py")
# folder = Path("C:/Users/James/Documents/Programs/Python3/UniversityProject/")
folder = getProjectFolder()
########################################################################################################################
# remove shapes.txt
shapes = folder / "shapes.txt"
if shapes.exists():
    shapes.unlink()
# remove mesh.mesh
mesh = folder / "mesh.mesh"
if mesh.exists():
    mesh.unlink()
# remove mesh.su2
mesh = folder / "mesh.su2"
if mesh.exists():
    mesh.unlink()
########################################################################################################################