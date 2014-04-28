-- two boxes
function simplescene()
  local groundbox = box(50.0, 4.0, 50.0, mat_simple_index)
  local ground = translation(0.0, -3.0, 0.0, groundbox)
  local small = box(5.0, 2.0, 5.0, mat_simple_index)
  return U(translation(10.0, 0.0, 10.0, small), ground);
end
setfenv(simplescene, csg)

function capped_cylinder(x, z, r, ymin, ymax, matindex)
  local cyl = cylinderxz(x, z, r, matindex)
  local plane0 = plane(0.0, 1.0,0.0,-ymin, matindex)
  local plane1 = plane(0.0,-1.0,0.0, ymax, matindex)
  local ccyl = D(D(cyl, plane0), plane1);
  ccyl:setmin(x-r,ymin,z-r)
  ccyl:setmax(x+r,ymax,z+r)
  return ccyl;
end
setfenv(capped_cylinder, csg)

-- build a very simple arcade
function arcade()
  local big = box(3.0, 4.0, 20.0, mat_simple_index);
  local b = box(2.0, 2.0, 20.0, mat_simple_index);
  local cut = translation(0.0, -2.0, 0.0, b);
  big = D(big, cut)
  local cxy = cylinderxy(0.0, 0.0, 2.0, mat_simple_index);
  local arcade = translation(16.0, 4.0, 10.0, D(big, cxy));
  for i=0,6 do
    local hole = box(3.0,1.0,1.0, mat_simple_index)
    arcade = D(arcade, translation(16.0,3.5,7.0+3.0*i,hole));
  end
  return arcade
end
setfenv(arcade, csg)

-- more complex with more than one material
function complexscene()
  -- a bunch of cylinder with a hole in the middle
  local s = sphere(4.2, mat_simple_index);
  local b0 = rotation(0.0, 25.0, 0.0, box(4.0,4.0,4.0,mat_simple_index))
  local d0 = translation(7.0, 5.0, 7.0, s);
  local d1 = translation(7.0, 5.0, 7.0, b0);
  local c = D(d1, d0);
  for i=0,15 do
    c = U(c, capped_cylinder(2.0, 2.0+2.0*i, 1.0, 1.0, 2*i+2.0, mat_snoise_index))
  end
  local b = box(3.5, 4.0, 3.5, mat_simple_index);
  local scene0 = D(c, translation(2.0,5.0,18.0, b));

  -- build the simili arcades
  local arc = arcade()

  -- add a ground box
  local groundbox = box(50.0, 4.0, 50.0, mat_simple_index)
  local ground = translation(0.0, -3.0, 0.0, groundbox)

  -- just make a union of them
  local world = U(ground, U(scene0, arc))

  -- add nested cylinders
  local c0 = capped_cylinder(30.0,30.0,2.0,0.0,2.0, mat_simple_index)
  local c1 = capped_cylinder(30.0,30.0,1.0,0.0,2.0, mat_simple_index)
  local c2 = capped_cylinder(30.0,30.0,1.0,0.0,2.0, mat_snoise_index)
  c0 = U(D(c0, c1), c2);

  -- change the material just to see
  local remove = translation(18.0,3.0,7.0, sphere(4.2, mat_simple_index))
  local newmat0 = translation(30.0,3.0,7.0, cylinderxz(0,0,4.2, mat_snoise_index))
  local newmat1 = translation(30.0,3.0,10.0, cylinderxz(0,0,4.2, mat_snoise_index));
  return U(c0, D(R(R(world, newmat0), newmat1), remove))
  --return d0
 -- return U(scene0, arc);
end

local env = {
  capped_cylinder=capped_cylinder,
  arcade=arcade
}
setmetatable(env, {__index=csg})
setfenv(complexscene, env)

-- create our scene
local scene, root = "complexscene", nil
if scene == "simplescene" then
  root = simplescene();
elseif scene == "complexscene" then
  root = complexscene();
end
csg.setroot(root)

