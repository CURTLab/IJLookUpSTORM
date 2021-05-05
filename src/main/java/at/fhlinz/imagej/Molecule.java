/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package at.fhlinz.imagej;

/**
 *
 * @author A41316
 */
public class Molecule {
    public int frame; // frame starting at 0
    public double x; // position in nm
    public double y; // position in nm
    public double z; // position in nm
    public double intensity; // number of photon emitted by the molecule
    public double background; // number of mean background photons
    public double crlb_x; // Cramér–Rao lower bound in nm or NaN
    public double crlb_y; // Cramér–Rao lower bound in nm or NaN
    public double crlb_z; // Cramér–Rao lower bound in nm or NaN
    
    public Molecule()
    {
        frame = 0;
        x = 0;
        y = 0;
        z = 0;
        intensity = 0;
        background = 0;
        crlb_x = Double.NaN;
        crlb_y = Double.NaN;
        crlb_z = Double.NaN;
    }
    
    @Override
    public String toString() {   
        return "Mol(" + x + " nm/" + y + " nm/" + z + " nm)";
    }
}
