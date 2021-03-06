function [hopper] = BuildInteractiveHopperSolution(varargin)
% This function is used to build an InteractiveHopper GUI solution; see
% BuildHopper and BuildDevice to build the hopper model on your own.

%-----------------------------------------------------------------------%
% The OpenSim API is a toolkit for musculoskeletal modeling and         %
% simulation. See http://opensim.stanford.edu and the NOTICE file       %
% for more information. OpenSim is developed at Stanford University     %
% and supported by the US National Institutes of Health (U54 GM072970,  %
% R24 HD065690) and by DARPA through the Warrior Web program.           %
%                                                                       %
% Copyright (c) 2017 Stanford University and the Authors                %
% Author(s): Nick Bianco, Carmichael Ong                                %
%                                                                       %
% Licensed under the Apache License, Version 2.0 (the "License");       %
% you may not use this file except in compliance with the License.      %
% You may obtain a copy of the License at                               %
% http://www.apache.org/licenses/LICENSE-2.0.                           %
%                                                                       %
% Unless required by applicable law or agreed to in writing, software   %
% distributed under the License is distributed on an "AS IS" BASIS,     %
% WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or       %
% implied. See the License for the specific language governing          %
% permissions and limitations under the License.                        %
%-----------------------------------------------------------------------%

%% INPUT PARSING 

% Create input parser
p = inputParser();

% Default values if no input is specified
defaultMuscle = 'averageJoe';
defaultMuscleExcitation = [0.0 1.99 2.0 3.89 3.9 4.0;
                          0.3 0.3  1.0 1.0  0.1 0.1];
defaultAddPassiveDevice = false;
defaultPassivePatellaWrap = false;
defaultPassiveParameter = 50;
defaultAddActiveDevice = false;
defaultActivePatellaWrap = false;
defaultIsActivePropMyo = false;
defaultActiveParameter = 50;
defaultDeviceControl = [0.0 2.5 5.0;
                        0.0 0.75 0.0];
defaultPrintSubcomponentAndOutputInfo = true;

% Create optional values for input parser                   
addOptional(p,'muscle',defaultMuscle)
addOptional(p,'muscleExcitation',defaultMuscleExcitation)
addOptional(p,'addPassiveDevice',defaultAddPassiveDevice)
addOptional(p,'passivePatellaWrap',defaultPassivePatellaWrap)
addOptional(p,'passiveParameter',defaultPassiveParameter)
addOptional(p,'addActiveDevice',defaultAddActiveDevice)
addOptional(p,'activePatellaWrap',defaultActivePatellaWrap)
addOptional(p,'isActivePropMyo',defaultIsActivePropMyo)
addOptional(p,'activeParameter',defaultActiveParameter)
addOptional(p,'deviceControl',defaultDeviceControl)
addOptional(p,'printSubcomponentAndOutputInfo',defaultPrintSubcomponentAndOutputInfo)

% Parse inputs
parse(p,varargin{:});

% Retrieve inputs and/or default values
muscle = p.Results.muscle;
muscleExcitation = p.Results.muscleExcitation;
addPassiveDevice = p.Results.addPassiveDevice;
passivePatellaWrap = p.Results.passivePatellaWrap;
passiveParameter = p.Results.passiveParameter;
addActiveDevice = p.Results.addActiveDevice;
activePatellaWrap = p.Results.activePatellaWrap;
isActivePropMyo = p.Results.isActivePropMyo;
activeParameter = p.Results.activeParameter;
deviceControl = p.Results.deviceControl;
printSubcomponentAndOutputInfo = p.Results.printSubcomponentAndOutputInfo;

import org.opensim.modeling.*;

%% HOPPER AND DEVICE SETTINGS

additionalMass = 0;

% Retrieve muscle settings based on user selection 
%   default: "The Average Joe"
[muscleFunc] = InteractiveHopperParameters(muscle);
[maxIsometricForce,optimalFiberLength,tendonSlackLength,muscleMass] = muscleFunc();
additionalMass = additionalMass + muscleMass;

% Retreive passive device settings if passive device specified
%   default: no passive device
%            if device --> passiveParameter = 50
if addPassiveDevice
    passive = InteractiveHopperParameters('passive');
    [passiveMass,springStiffness] = passive(passiveParameter);
    additionalMass = additionalMass + passiveMass;
end

% Retreive passive device settings if passive device specified
%   default: no active device
%            if device --> activeParameter = 50
if addActiveDevice
    if isActivePropMyo
        activePropMyo = InteractiveHopperParameters('activePropMyo');
        [activeMass,gain] = activePropMyo(activeParameter);
    else
        activeControl = InteractiveHopperParameters('activeControl');
        [activeMass,maxTension] = activeControl(activeParameter);
    end
    additionalMass = additionalMass + activeMass;
end

%% BUILD HOPPER MODEL

% Build hopper
hopper = BuildHopper('excitation',muscleExcitation, ...
                     'additionalMass',additionalMass, ...
                     'maxIsometricForce', maxIsometricForce, ...
                     'optimalFiberLength', optimalFiberLength, ...
                     'tendonSlackLength', tendonSlackLength);
if printSubcomponentAndOutputInfo
    hopper.printSubcomponentInfo();
end

%% BUILD DEVICES
devices = cell(0);
deviceNames = cell(0);
patellaWrap = cell(0);

% Passive device
if addPassiveDevice
    passive = BuildDevice('deviceType','passive', ... 
                          'springStiffness',springStiffness);
    devices{1,length(devices)+1} = passive;
    deviceNames{1,length(deviceNames)+1} = 'device_passive';
    patellaWrap{1,length(patellaWrap)+1} = passivePatellaWrap;  
end

% Active device
if addActiveDevice
    if isActivePropMyo
        active = BuildDevice('deviceType','active', ...
                             'isPropMyo',true, ...
                             'gain',gain);
    else
        active = BuildDevice('deviceType','active', ...
                             'isPropMyo',false, ... 
                             'control',deviceControl, ...
                             'maxTension',maxTension);
    end
    
    devices{1,length(devices)+1} = active;
    deviceNames{1,length(deviceNames)+1} = 'device_active';
    patellaWrap{1,length(patellaWrap)+1} = activePatellaWrap;   
end

%% CONNECT DEVICES TO HOPPER
for d = 1:length(devices)
    
    % Print the names of the device's subcomponents, and locate the
    % subcomponents named 'anchorA' and 'anchorB'. Also, print the names of
    % the hopper's subcomponents, and locate the two subcomponents named
    % 'deviceAttach'.
    device = devices{d};
    if printSubcomponentAndOutputInfo
        device.printSubcomponentInfo();
    end
    
    % Add the device to the hopper model.
    hopper.addComponent(device);
    
    % Get the 'anchor' joints in the device, and downcast them to the
    % WeldJoint class. Get the 'deviceAttach' frames in the hopper
    % model, and downcast them to the PhysicalFrame class.
    anchorA = WeldJoint.safeDownCast(device.updComponent('anchorA'));
    anchorB = WeldJoint.safeDownCast(device.updComponent('anchorB'));
    thighAttach = PhysicalFrame.safeDownCast(...
        hopper.getComponent('thigh/deviceAttach'));
    shankAttach = PhysicalFrame.safeDownCast(...
        hopper.getComponent('shank/deviceAttach'));
    
    % Connect the parent frame sockets of the device's anchor joints to the
    % attachment frames on the hopper; attach anchorA to the thigh, and
    % anchorB to the shank.
    anchorA.connectSocket_parent_frame(thighAttach);
    anchorB.connectSocket_parent_frame(shankAttach);
    
    % Configure the device to wrap over the patella.
    if patellaWrap{d} && hopper.hasComponent([deviceNames{d}])
        if strcmp(deviceNames{d},'device_passive')
            % TODO: Change to downcast from PathSpring when PathSpring is
            % working for passive device
            cable = PathActuator.safeDownCast(device.updComponent('cableAtoBpassive'));
        elseif strcmp(deviceNames{d},'device_active')
            cable = PathActuator.safeDownCast(device.updComponent('cableAtoBactive'));
        end
        patellaPath = 'thigh/patellaFrame/patella';
        wrapObject = WrapCylinder.safeDownCast(hopper.updComponent(patellaPath));
        cable.updGeometryPath().addPathWrap(wrapObject);
    end
    
    % Print the names of the outputs of the device's PathActuator and
    % ToyPropMyoController subcomponents.
    if strcmp(deviceNames{d},'device_active') && printSubcomponentAndOutputInfo
        device.getComponent('cableAtoBactive').printOutputInfo();
        device.getComponent('controller').printOutputInfo();
    end
    
    % Use the vastus muscle's activation output as the ToyPropMyoController's
    % control input.
    if strcmp(deviceNames{d},'device_active') && isActivePropMyo
        device.updComponent('controller').updInput('activation').connect(...
            hopper.getComponent('vastus').getOutput('activation'));
    end
    
end

end
