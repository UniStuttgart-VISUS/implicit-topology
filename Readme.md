
![](logo.png)


MegaMol is a visualization middleware used to visualize point-based molecular data sets. This software is developed within the ​Collaborative Research Center 716, subproject ​D.3 at the ​[Visualization Research Center (VISUS)](https://www.visus.uni-stuttgart.de/en) of the University of Stuttgart and at the ​Computer Graphics and Visualization Group of the TU Dresden.  

MegaMol succeeds ​MolCloud, which has been developed at the University of Stuttgart in order to visualize point-based data sets. MegaMol is written in C++, and uses an OpenGL as Rendering-API and GLSL-Shader. It supports the operating systems Microsoft Windows and Linux, each in 32-bit and 64-bit versions. In large parts, MegaMol is based on VISlib, a C++-class library for scientific visualization, which has also been developed at the University of Stuttgart. 


## Building MegaMol
### Linux
1. Clone the MegaMol repository
2. Create a build folder
3. Invoke `cmake` inside the build folder
    1. Set the `cmake` variable `BUILD_FLOWVIS_PLUGIN` to `ON`
4. Execute `make` to build MegaMol
5. Run `make install` to create your MegaMol installation
6. Test Megamol with

        ./megamol.sh -p ..\examples\testspheres.lua

### Windows
1. Clone the MegaMol repository
2. Use the cmake GUI to configure MegaMol
    1. Make sure to enable the *flowvis* plugin by checking `BUILD_FLOWVIS_PLUGIN`
    2. The configuration creates an `sln` file inside the build folder
3. Open the `sln` file with *Visual Studio*
4. Use the `ALL_BUILD` target to build MegaMol
5. Use the `INSTALL` target to create your MegaMol installation
6. Test Megamol with

        mmconsole.exe -p ..\examples\testspheres.lua


## MegaMol Configurator
MegaMol offers a configurator GUI (C#) that runs with .Net Framework 4.
It runs also on Linux with Mono 3.2.8 (except for the analysis function and indirect-start functions).  


## Implicit Topology Computation and Visualization
The files necessary to reproduce the results from our paper


**Implicit Visualization of 2D Vector Field Topology for Periodic Orbit Detection**  
A. Straub, G. K. Karch, F. Sadlo, T. Ertl  
Proceedings of TopoInVis, 2019

    @InProceedings{straub2019implicit,
        author    = {Straub, Alexander and Karch, Grzegorz K. and Sadlo, Filip and Ertl, Thomas},
        booktitle = {Proceedings of TopoInVis},
        title     = {Implicit Visualization of {2D} Vector Field Topology for Periodic Orbit Detection},
        year      = {2019}
    }


are located in the `projects` folder in the source directory of MegaMol. To run the project in MegaMol, run one of the following commands from within the `projects` folder:


    path/to/mmconsole -p "topoinvis_buoyant_I.mmprj" -i TopoInVis_(Buoyant_I) inst
    path/to/mmconsole -p "topoinvis_buoyant_II.mmprj" -i TopoInVis_(Buoyant_II) inst
    path/to/mmconsole -p "theisel.mmprj" -i Theisel3D inst


Using the GUI, you can modify the parameters, run the computations, and visualize the results. To just load the results provided in the data folder, expand `::inst::implicit_topology1` in the GUI, press the `load_computation` button, and follow up by starting the computation by clicking on `start_computation`.


## Citing MegaMol
Please use one of the following methods to reference the MegaMol project.


**MegaMol – A Comprehensive Prototyping Framework for Visualizations**  
P. Gralka, M. Becher, M. Braun, F. Frieß, C. Müller, T. Rau, K. Schatz, C. Schulz, M. Krone, G. Reina, T. Ertl  
The European Physical Journal Special Topics, vol. 227, no. 14, pp. 1817--1829, 2019  
doi: 10.1140/epjst/e2019-800167-5

    @article{gralka2019megamol,
        author={Gralka, Patrick
        and Becher, Michael
        and Braun, Matthias
        and Frie{\ss}, Florian
        and M{\"u}ller, Christoph
        and Rau, Tobias
        and Schatz, Karsten
        and Schulz, Christoph
        and Krone, Michael
        and Reina, Guido
        and Ertl, Thomas},
        title={{MegaMol -- A Comprehensive Prototyping Framework for Visualizations}},
        journal={The European Physical Journal Special Topics},
        year={2019},
        month={Mar},
        volume={227},
        number={14},
        pages={1817--1829},
        issn={1951-6401},
        doi={10.1140/epjst/e2019-800167-5},
        url={https://doi.org/10.1140/epjst/e2019-800167-5}
    }
#
**MegaMol – A Prototyping Framework for Particle-based Visualization**  
S. Grottel, M. Krone, C. Müller, G. Reina, T. Ertl  
Visualization and Computer Graphics, IEEE Transactions on, vol.21, no.2, pp. 201--214, Feb. 2015  
doi: 10.1109/TVCG.2014.2350479

    @article{grottel2015megamol,
      author={Grottel, S. and Krone, M. and Muller, C. and Reina, G. and Ertl, T.},
      journal={Visualization and Computer Graphics, IEEE Transactions on},
      title={MegaMol -- A Prototyping Framework for Particle-based Visualization},
      year={2015},
      month={Feb},
      volume={21},
      number={2},
      pages={201--214},
      keywords={Data models;Data visualization;Graphics processing units;Libraries;Rendering(computer graphics);Visualization},
      doi={10.1109/TVCG.2014.2350479},
      ISSN={1077-2626}
    }
#
**Coherent Culling and Shading for Large Molecular Dynamics Visualization**  
S. Grottel, G. Reina, C. Dachsbacher, T. Ertl  
Computer Graphics Forum (Proceedings of EUROVIS 2010), 29(3):953 - 962, 2010

    @article{eurovis10-grottel,
      author = {Grottel, S. and Reina, G. and Dachsbacher, C. and Ertl, T.},
      title  = {{Coherent Culling and Shading for Large Molecular Dynamics Visualization}},
      url    = {https://go.visus.uni-stuttgart.de/megamol},
      year   = {2010},
      pages  = {953--962},
      journal = {{Computer Graphics Forum}},
      volume = {{29}},
      number = {{3}}
    }
#
**Optimized Data Transfer for Time-dependent, GPU-based Glyphs**  
S. Grottel, G. Reina, T. Ertl  
In Proceedings of IEEE Pacific Visualization Symposium 2009: 65 - 72, 2009

    @InProceedings{pvis09-grottel,
      author = {Grottel, S. and Reina, G. and Ertl, T.},
      title  = {{Optimized Data Transfer for Time-dependent, GPU-based Glyphs}},
      url    = {https://go.visus.uni-stuttgart.de/megamol},
      year   = {2009},
      pages  = {65-72},
      booktitle = {{Proceedings of IEEE Pacific Visualization Symposium 2009}}
    }

#
**MegaMol™ project website**  
[https://megamol.org](https://megamol.org)

    @misc{megamol,
      key  = "megamol",
      url  = {https://megamol.org},
      note = {{MegaMol project website \url{https://megamol.org}}},
    }
