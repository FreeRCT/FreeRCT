{
  description = "A free and open source game which captures the look, feel and gameplay of the popular games RollerCoaster Tycoon 1 and 2.";

  outputs = { self, nixpkgs }: {

    packages.x86_64-linux.default =
      with import nixpkgs { system = "x86_64-linux"; };
      stdenv.mkDerivation {
        name = "freertc";
        src = self;
        nativeBuildInputs = [git cmake];
        buildInputs = [libpng glfw glew freetype];
      };
  };
}
